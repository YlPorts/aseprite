package com.ylports.aseprite;

import android.app.Activity;
import android.content.pm.ActivityInfo;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.content.Context;
import android.text.InputType;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputMethodManager;
import java.io.File;
import java.io.InputStream;
import java.io.FileOutputStream;
import java.io.IOException;

/** Native Android host of the ORIGINAL Aseprite editor; never a substitute UI. */
public final class AsepriteActivity extends Activity implements SurfaceHolder.Callback {
    private static final int IMMERSIVE=0x00000002|0x00000004|0x00001000|0x00000200|0x00000400;
    static { System.loadLibrary("aseprite"); }
    private static volatile AsepriteActivity activeActivity;
    private SurfaceView canvas;

    // The original C++ LAF requests/hides text input. All Java UI work
    // stays on the Android main thread, not on Aseprite's C++ render thread.
    private static void setImeEnabledFromNative(boolean enabled) {
        AsepriteActivity activity=activeActivity;
        if(activity==null) return;
        activity.runOnUiThread(()->{
            if(activeActivity!=activity || activity.canvas==null || activity.isFinishing()) return;
            InputMethodManager manager=(InputMethodManager)
                activity.getSystemService(Context.INPUT_METHOD_SERVICE);
            if(manager==null) return;
            if(enabled){
                activity.canvas.requestFocus();
                manager.restartInput(activity.canvas);
                manager.showSoftInput(activity.canvas,InputMethodManager.SHOW_IMPLICIT);
            }else{
                manager.hideSoftInputFromWindow(activity.canvas.getWindowToken(),0);
            }
        });
    }

    // Android input connection feeds characters into Aseprite's authentic
    // os::Event queue; it never creates another text or sprite editor.
    private static final class EditorSurfaceView extends SurfaceView {
        EditorSurfaceView(Context context){super(context);}
        @Override public boolean onCheckIsTextEditor(){return true;}
        @Override public InputConnection onCreateInputConnection(EditorInfo info){
            info.inputType=InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_FLAG_MULTI_LINE;
            info.imeOptions=EditorInfo.IME_FLAG_NO_EXTRACT_UI;
            return new BaseInputConnection(this,false){
                @Override public boolean commitText(CharSequence text,int cursorPosition){
                    if(text==null) return false;
                    for(int i=0;i<text.length();){
                        int cp=Character.codePointAt(text,i);
                        nativeCommitText(cp);
                        i+=Character.charCount(cp);
                    }
                    return true;
                }
                @Override public boolean deleteSurroundingText(int before,int after){
                    if(before>0){
                        nativeKey(KeyEvent.ACTION_DOWN,KeyEvent.KEYCODE_DEL,0,0);
                        nativeKey(KeyEvent.ACTION_UP,KeyEvent.KEYCODE_DEL,0,0);
                    }
                    return true;
                }
                @Override public boolean sendKeyEvent(KeyEvent event){
                    nativeKey(event.getAction(),event.getKeyCode(),
                              event.getUnicodeChar(),event.getMetaState());
                    return true;
                }
            };
        }
    }
    private boolean nativeStarted=false;
    private boolean assetsReady=false;
    private boolean loadingAssets=false;

    private static native void nativeRegisterImeBridge();
    private static native void nativeCommitText(int codepoint);
    private static native void nativeAttachSurface(Surface surface);
    private static native boolean nativeStartOriginalEditor(String filesRoot);
    private static native void nativeTouch(int action,int pointerIndex,int[] ids,
                                            float[] xy,float[] pressure,int[] tools);
    private static native void nativeKey(int action,int keyCode,int unicode,int modifiers);
    private static native void nativeAppFocus(boolean focused);

    @Override public void onCreate(Bundle state) {
        super.onCreate(state);
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        getWindow().getDecorView().setSystemUiVisibility(IMMERSIVE);
        activeActivity=this;
        nativeRegisterImeBridge();
        canvas=new EditorSurfaceView(this);
        canvas.getHolder().addCallback(this);
        canvas.setFocusable(true);
        canvas.setFocusableInTouchMode(true);
        canvas.setOnTouchListener((view,event)->{
            int count=event.getPointerCount();
            if(count<1) return true;
            int[] ids=new int[count];
            int[] tools=new int[count];
            float[] xy=new float[count*2];
            float[] pressure=new float[count];
            for(int i=0;i<count;i++){
                ids[i]=event.getPointerId(i);
                tools[i]=event.getToolType(i);
                xy[i*2]=event.getX(i);
                xy[i*2+1]=event.getY(i);
                pressure[i]=event.getPressure(i);
            }
            nativeTouch(event.getActionMasked(),event.getActionIndex(),ids,xy,pressure,tools);
            return true;
        });
        setContentView(canvas);
    }

    @Override public void surfaceCreated(SurfaceHolder holder) {
        nativeAttachSurface(holder.getSurface());
        prepareOriginalResources();
        canvas.requestFocus();
    }
    @Override public void surfaceChanged(SurfaceHolder holder,int format,int width,int height) {
        nativeAttachSurface(holder.getSurface());
        maybeStartEngine();
    }
    @Override public void surfaceDestroyed(SurfaceHolder holder) {
        nativeAttachSurface(null);
    }
    private void maybeStartEngine(){
        if (assetsReady && !nativeStarted && canvas.getHolder().getSurface().isValid()) {
            File userDir=new File(getFilesDir(),"user");
            if (!userDir.isDirectory() && !userDir.mkdirs()) {
                android.util.Log.e("AsepriteAndroid","Cannot create private Aseprite user directory");
                return;
            }
            if (nativeStartOriginalEditor(getFilesDir().getAbsolutePath())) {
                nativeStarted=true;
            } else {
                android.util.Log.e("AsepriteAndroid",
                    "Original editor native startup was not accepted; will retry on surface event");
            }
        }
    }
    private void prepareOriginalResources(){
        if (assetsReady) { maybeStartEngine(); return; }
        if (loadingAssets) return;
        loadingAssets=true;
        new Thread(()->{
            IOException failure=null;
            try {
                String[] top=getAssets().list("data");
                if (top==null || top.length==0)
                    throw new IOException("Missing authentic Aseprite assets/data");
                copyAssets("data",new File(getFilesDir(),"data"));
            } catch (IOException error) { failure=error; }
            IOException finalFailure=failure;
            runOnUiThread(()->{
                loadingAssets=false;
                if(finalFailure!=null){
                    new android.app.AlertDialog.Builder(this).setTitle("Missing Aseprite resources")
                      .setMessage(finalFailure.toString())
                      .setPositiveButton("Close",(d,w)->finish()).show();
                }else{
                    assetsReady=true;
                    maybeStartEngine();
                }
            });
        },"AsepriteAssetInstall").start();
    }
    private void copyAssets(String assetPath,File dest) throws IOException {
        String[] children=getAssets().list(assetPath);
        if(children==null || children.length==0){
            // A process kill during the first install must not leave a
            // partially-written original Aseprite resource marked as ready.
            File parent=dest.getParentFile();
            if(parent==null || (!parent.isDirectory() && !parent.mkdirs()))
                throw new IOException("Cannot create "+parent);
            File temporary=new File(parent,dest.getName()+".installing");
            boolean installed=false;
            try(InputStream in=getAssets().open(assetPath);
                FileOutputStream out=new FileOutputStream(temporary)){
                byte[] bytes=new byte[65536];
                int n;
                while((n=in.read(bytes))!=-1)out.write(bytes,0,n);
                out.getFD().sync();
            } catch(IOException error){
                temporary.delete();
                throw error;
            }
            if(dest.exists() && !dest.delete()){
                temporary.delete();
                throw new IOException("Cannot replace original resource "+dest);
            }
            installed=temporary.renameTo(dest);
            if(!installed){
                temporary.delete();
                throw new IOException("Cannot finalize original resource "+dest);
            }
            return;
        }
        if(!dest.isDirectory() && !dest.mkdirs())
            throw new IOException("Cannot create "+dest);
        for(String child: children) copyAssets(assetPath+"/"+child,new File(dest,child));
    }
    @Override public boolean dispatchKeyEvent(KeyEvent event){
        int action=event.getAction();
        if(action==KeyEvent.ACTION_DOWN || action==KeyEvent.ACTION_UP){
            nativeKey(action,event.getKeyCode(),event.getUnicodeChar(),event.getMetaState());
            return true;
        }
        return super.dispatchKeyEvent(event);
    }
    @Override protected void onPause(){
        nativeAppFocus(false);
        super.onPause();
    }
    @Override protected void onResume(){
        super.onResume();
        nativeAppFocus(true);
        getWindow().getDecorView().setSystemUiVisibility(IMMERSIVE);
    }
    @Override protected void onDestroy(){
        if(activeActivity==this) activeActivity=null;
        super.onDestroy();
    }
}
