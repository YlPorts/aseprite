package org.ylports.aseprite;

import android.app.*;
import android.os.*;
import android.content.*;
import android.graphics.*;
import android.net.Uri;
import android.util.AtomicFile;
import android.view.*;
import android.widget.*;
import java.io.*;
import java.util.concurrent.*;

/** Experimental Android frontend to upstream doc/render/dio; not the desktop UI. */
public final class MainActivity extends Activity {
    private static final int OPEN=10, SAVE=11;
    private long core;
    private TextView status;
    private PixelView canvas;
    private boolean eraser, playing, dirty, closed;
    private int ink=0xffe9edf4, brush=1;
    private int[] meta;
    private long revision;
    private byte[] pendingSave;
    private boolean pendingProject;
    private long pendingRevision;
    private final Handler handler=new Handler(Looper.getMainLooper());
    private final ExecutorService io=Executors.newSingleThreadExecutor();
    private final Runnable playback=new Runnable(){ public void run(){
        if(!playing||closed)return;
        try{NativeCore.command(core,4,(meta[3]+1)%meta[2]);refresh();handler.postDelayed(this,Math.max(20,meta[6]));}catch(Throwable e){stop();error(e);}
    }};
    @Override public void onCreate(Bundle state){
        super.onCreate(state);
        try{
            core=NativeCore.create(64,64);
            LinearLayout root=new LinearLayout(this);root.setOrientation(1);root.setBackgroundColor(0xff161a22);
            TextView title=new TextView(this);title.setText("ASEPRITE CORE  /  ANDROID ALPHA");title.setTextSize(16);title.setPadding(dp(16),dp(12),dp(16),dp(8));title.setTextColor(0xffb5c9ff);root.addView(title);
            LinearLayout files=row(root);
            button(files,"Nuevo",()->confirmReplace(this::newCanvas));button(files,"Abrir",()->confirmReplace(this::openPicker));
            button(files,"Guardar copia",()->savePicker(true));button(files,"PNG",()->savePicker(false));
            LinearLayout tools=row(root);
            button(tools,"Lapiz",()->{eraser=false;showTool();});button(tools,"Goma",()->{eraser=true;showTool();});
            button(tools,"Deshacer",()->operation(0,0,true));button(tools,"Rehacer",()->operation(1,0,true));
            button(tools,"Grosor",()->new AlertDialog.Builder(this).setTitle("Grosor en pixeles").setItems(new String[]{"1","2","3","4","8","16","32"},(d,n)->{brush=new int[]{1,2,3,4,8,16,32}[n];showTool();}).show());
            LinearLayout colors=row(root);
            for(int c:new int[]{0xff10131a,0xffffffff,0xffe74766,0xffffa24a,0xffffdf6b,0xff5dd597,0xff57b9ee,0xff8277ef,0xffd985ce}){
                Button b=button(colors," ",()->{ink=c;eraser=false;showTool();});b.setBackgroundTintList(android.content.res.ColorStateList.valueOf(c));b.setContentDescription(String.format("Color #%06X",c&0xffffff));b.setMinWidth(0);b.setMinimumWidth(0);b.setLayoutParams(new LinearLayout.LayoutParams(dp(48),dp(44)));
            }
            button(colors,"HEX",this::chooseColor);
            canvas=new PixelView();root.addView(canvas,new LinearLayout.LayoutParams(-1,0,1));
            status=new TextView(this);status.setTextSize(12);status.setTextColor(0xffc1c8d9);status.setPadding(dp(12),dp(6),dp(12),dp(6));root.addView(status);
            LinearLayout timeline=row(root);
            button(timeline,"Anterior",()->operation(4,(meta[3]+meta[2]-1)%meta[2],false));button(timeline,"Play / Stop",()->{if(playing)stop();else{playing=true;handler.postDelayed(playback,meta[6]);}});
            button(timeline,"Siguiente",()->operation(4,(meta[3]+1)%meta[2],false));button(timeline,"+ Frame",()->operation(2,0,true));button(timeline,"Capas",this::layers);button(timeline,"Duracion",this::duration);button(timeline,"Centrar",()->{canvas.fit();canvas.invalidate();});
            setContentView(root);
            root.setOnApplyWindowInsetsListener((v,insets)->{if(Build.VERSION.SDK_INT>=30){Insets i=insets.getInsets(WindowInsets.Type.systemBars()|WindowInsets.Type.displayCutout());v.setPadding(i.left,i.top,i.right,i.bottom);}return insets;});
            refresh();
            File recovery=new File(getFilesDir(),"recovery.aseprite");
            if(recovery.isFile())new AlertDialog.Builder(this).setTitle("Recuperar trabajo").setMessage("Hay una copia local de la ultima sesion. No sustituye tus archivos guardados.").setPositiveButton("Recuperar",(d,w)->loadFile(recovery)).setNegativeButton("Nuevo",null).show();
        }catch(Throwable e){TextView fail=new TextView(this);fail.setPadding(24,48,24,24);fail.setText("No se pudo iniciar el nucleo nativo.\n\n"+e);setContentView(fail);}
    }
    private int dp(float n){return Math.round(n*getResources().getDisplayMetrics().density);}
    private LinearLayout row(LinearLayout parent){HorizontalScrollView sc=new HorizontalScrollView(this);sc.setHorizontalScrollBarEnabled(false);LinearLayout r=new LinearLayout(this);r.setPadding(dp(6),0,dp(6),0);sc.addView(r);parent.addView(sc,new LinearLayout.LayoutParams(-1,dp(48)));return r;}
    private Button button(LinearLayout row,String text,Runnable action){Button b=new Button(this);b.setText(text);b.setTextSize(12);b.setAllCaps(false);b.setOnClickListener(v->{try{action.run();}catch(Throwable e){error(e);}});row.addView(b,new LinearLayout.LayoutParams(-2,dp(48)));return b;}
    private void error(Throwable e){if(!closed&&!isFinishing()&&!isDestroyed())new AlertDialog.Builder(this).setTitle("Aseprite Core Alpha").setMessage(e.getMessage()==null?e.toString():e.getMessage()).setPositiveButton("Cerrar",null).show();}
    private void stop(){playing=false;handler.removeCallbacks(playback);}
    private void changed(){dirty=true;revision++;}
    private void refresh(){meta=NativeCore.info(core);int[] pixels=NativeCore.render(core);if(canvas.bitmap==null||canvas.bitmap.getWidth()!=meta[0]||canvas.bitmap.getHeight()!=meta[1])canvas.bitmap=Bitmap.createBitmap(meta[0],meta[1],Bitmap.Config.ARGB_8888);canvas.bitmap.setPixels(pixels,0,meta[0],0,0,meta[0],meta[1]);canvas.invalidate();showTool();}
    private void showTool(){if(status!=null&&meta!=null)status.setText(meta[0]+" x "+meta[1]+"  |  Frame "+(meta[3]+1)+"/"+meta[2]+"  |  Capa "+(meta[5]+1)+"/"+meta[4]+"  |  "+(eraser?"Goma":"Lapiz")+" "+brush+" px"+(dirty?"  *":""));}
    private void operation(int op,int val,boolean edit){stop();if(NativeCore.command(core,op,val)&&edit)changed();refresh();}
    private void confirmReplace(Runnable next){stop();if(!dirty){next.run();return;}new AlertDialog.Builder(this).setTitle("Trabajo sin guardar").setMessage("Guarda una copia antes de cambiar de documento. Continuar descarta el documento actual.").setPositiveButton("Continuar",(d,w)->next.run()).setNegativeButton("Volver",null).show();}
    private void newCanvas(){final EditText size=new EditText(this);size.setText("64x64");size.setSingleLine(true);new AlertDialog.Builder(this).setTitle("Nuevo lienzo: ancho x alto (max. 512)").setView(size).setPositiveButton("Crear",(d,w)->{try{String[] p=size.getText().toString().toLowerCase().trim().split("x");if(p.length!=2)throw new IllegalArgumentException("Usa por ejemplo 64x64");long replacement=NativeCore.create(Integer.parseInt(p[0].trim()),Integer.parseInt(p[1].trim()));NativeCore.destroy(core);core=replacement;dirty=false;revision++;refresh();canvas.fit();}catch(Throwable e){error(e);}}).setNegativeButton("Cancelar",null).show();}
    private void chooseColor(){EditText text=new EditText(this);text.setSingleLine(true);text.setText(String.format("#%06X",ink&0xffffff));new AlertDialog.Builder(this).setTitle("Color #RRGGBB o #AARRGGBB").setView(text).setPositiveButton("Usar",(d,w)->{try{ink=Color.parseColor(text.getText().toString().trim());eraser=false;showTool();}catch(Throwable e){error(e);}}).setNegativeButton("Cancelar",null).show();}
    private void layers(){stop();String[] names=NativeCore.layers(core).split("\n");new AlertDialog.Builder(this).setTitle("Capas").setSingleChoiceItems(names,meta[5],(d,n)->{operation(5,n,false);d.dismiss();}).setPositiveButton("+ Capa",(d,n)->{try{operation(3,0,true);}catch(Throwable e){error(e);}}).setNeutralButton("Mostrar / ocultar actual",(d,n)->{try{operation(6,0,true);}catch(Throwable e){error(e);}}).setNegativeButton("Cerrar",null).show();}
    private void duration(){EditText text=new EditText(this);text.setInputType(2);text.setText(""+meta[6]);new AlertDialog.Builder(this).setTitle("Duracion del frame (20-5000 ms)").setView(text).setPositiveButton("Aplicar",(d,w)->{try{operation(7,Integer.parseInt(text.getText().toString()),true);}catch(Throwable e){error(e);}}).setNegativeButton("Cancelar",null).show();}
    private void openPicker(){Intent i=new Intent(Intent.ACTION_OPEN_DOCUMENT).setType("*/*").addCategory(Intent.CATEGORY_OPENABLE);i.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION|Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);startActivityForResult(i,OPEN);}
    private void savePicker(boolean project){stop();try{pendingProject=project;pendingRevision=revision;if(project)pendingSave=NativeCore.save(core);else{ByteArrayOutputStream out=new ByteArrayOutputStream();if(!canvas.bitmap.compress(Bitmap.CompressFormat.PNG,100,out))throw new IOException("No se pudo exportar PNG");pendingSave=out.toByteArray();}Intent i=new Intent(Intent.ACTION_CREATE_DOCUMENT).addCategory(Intent.CATEGORY_OPENABLE).setType(project?"application/octet-stream":"image/png").putExtra(Intent.EXTRA_TITLE,project?"sprite.aseprite":"frame.png");startActivityForResult(i,SAVE);}catch(Throwable e){error(e);}}
    @Override protected void onActivityResult(int req,int result,Intent data){super.onActivityResult(req,result,data);if(result!=RESULT_OK||data==null||data.getData()==null){pendingSave=null;return;}Uri uri=data.getData();if(req==OPEN){try{getContentResolver().takePersistableUriPermission(uri,data.getFlags()&Intent.FLAG_GRANT_READ_URI_PERMISSION);}catch(SecurityException ignored){}io.execute(()->{try(InputStream in=getContentResolver().openInputStream(uri)){byte[] bytes=readLimited(in);runOnUiThread(()->applyOpened(bytes));}catch(Throwable e){runOnUiThread(()->error(e));}});}else if(req==SAVE){final byte[] bytes=pendingSave;final boolean project=pendingProject;final long version=pendingRevision;pendingSave=null;if(bytes==null){error(new IOException("La exportacion se interrumpio; vuelve a guardar"));return;}io.execute(()->{try(OutputStream out=getContentResolver().openOutputStream(uri,"wt")){if(out==null)throw new IOException("Destino no disponible");out.write(bytes);out.flush();runOnUiThread(()->{if(project&&revision==version)dirty=false;showTool();Toast.makeText(this,"Archivo guardado",Toast.LENGTH_SHORT).show();});}catch(Throwable e){runOnUiThread(()->error(e));}});}}
    private static byte[] readLimited(InputStream in)throws IOException{if(in==null)throw new IOException("Archivo no disponible");ByteArrayOutputStream out=new ByteArrayOutputStream();byte[] b=new byte[32768];int n;while((n=in.read(b))!=-1){if(out.size()+n>32*1024*1024)throw new IOException("Limite alpha: 32 MB por archivo");out.write(b,0,n);}return out.toByteArray();}
    private void applyOpened(byte[] bytes){if(closed)return;try{NativeCore.open(core,bytes);dirty=false;revision++;refresh();canvas.fit();}catch(Throwable e){error(e);}}
    private void loadFile(File f){io.execute(()->{try(InputStream in=new FileInputStream(f)){byte[] bytes=readLimited(in);runOnUiThread(()->applyOpened(bytes));}catch(Throwable e){runOnUiThread(()->error(e));}});}
    @Override protected void onPause(){stop();if(core!=0&&!closed){try{byte[] bytes=NativeCore.save(core);AtomicFile file=new AtomicFile(new File(getFilesDir(),"recovery.aseprite"));io.execute(()->{FileOutputStream out=null;try{out=file.startWrite();out.write(bytes);file.finishWrite(out);}catch(Throwable e){if(out!=null)file.failWrite(out);}});}catch(Throwable ignored){}}super.onPause();}
    @Override protected void onDestroy(){closed=true;stop();if(core!=0){NativeCore.destroy(core);core=0;}io.shutdown();super.onDestroy();}
    @Override public void onBackPressed(){if(dirty)new AlertDialog.Builder(this).setTitle("Cerrar editor").setMessage("Quedara una copia de recuperacion local. Para conservar el proyecto fuera de la app, usa Guardar copia.").setPositiveButton("Cerrar",(d,w)->finish()).setNegativeButton("Seguir",null).show();else super.onBackPressed();}

    private final class PixelView extends View {
        Bitmap bitmap;Paint paint=new Paint();float scale=1,ox,oy,lastCx,lastCy;int lastX,lastY;boolean stroke,multi;ScaleGestureDetector zoom;
        PixelView(){super(MainActivity.this);setContentDescription("Lienzo de pixel art. Un dedo dibuja; dos dedos desplazan y amplian.");setFocusable(true);paint.setFilterBitmap(false);zoom=new ScaleGestureDetector(MainActivity.this,new ScaleGestureDetector.SimpleOnScaleGestureListener(){@Override public boolean onScale(ScaleGestureDetector d){float old=scale;scale=Math.max(0.25f,Math.min(128f,scale*d.getScaleFactor()));float f=scale/old;ox=d.getFocusX()-(d.getFocusX()-ox)*f;oy=d.getFocusY()-(d.getFocusY()-oy)*f;invalidate();return true;}});}
        void fit(){if(bitmap==null||getWidth()==0)return;scale=Math.max(0.25f,Math.min((getWidth()-dp(24))/(float)bitmap.getWidth(),(getHeight()-dp(24))/(float)bitmap.getHeight()));ox=(getWidth()-bitmap.getWidth()*scale)/2;oy=(getHeight()-bitmap.getHeight()*scale)/2;invalidate();}
        @Override protected void onSizeChanged(int w,int h,int ow,int oh){fit();}
        @Override protected void onDraw(Canvas c){c.drawColor(0xff0e1117);if(bitmap==null)return;float right=ox+bitmap.getWidth()*scale,bottom=oy+bitmap.getHeight()*scale;c.save();c.clipRect(ox,oy,right,bottom);int tile=dp(12);for(int y=0;y<getHeight();y+=tile)for(int x=0;x<getWidth();x+=tile){paint.setColor(((x/tile+y/tile)&1)==0?0xff414751:0xff303641);c.drawRect(x,y,x+tile,y+tile,paint);}paint.setAlpha(255);c.drawBitmap(bitmap,null,new RectF(ox,oy,right,bottom),paint);c.restore();paint.setColor(0xff65718c);paint.setStyle(Paint.Style.STROKE);paint.setStrokeWidth(dp(1));c.drawRect(ox,oy,right,bottom,paint);paint.setStyle(Paint.Style.FILL);}
        private int px(float x){return (int)Math.floor((x-ox)/scale);}private int py(float y){return (int)Math.floor((y-oy)/scale);}
        @Override public boolean onTouchEvent(android.view.MotionEvent e){try{
            final int action=e.getActionMasked();
            if(action==MotionEvent.ACTION_POINTER_DOWN){if(stroke){NativeCore.command(core,0,0);stroke=false;refresh();}multi=true;lastCx=(e.getX(0)+e.getX(1))/2;lastCy=(e.getY(0)+e.getY(1))/2;}
            zoom.onTouchEvent(e);
            if(e.getPointerCount()>=2){float cx=(e.getX(0)+e.getX(1))/2,cy=(e.getY(0)+e.getY(1))/2;if(action==MotionEvent.ACTION_MOVE){ox+=cx-lastCx;oy+=cy-lastCy;invalidate();}lastCx=cx;lastCy=cy;return true;}
            if(action==MotionEvent.ACTION_DOWN){stop();multi=false;int x=px(e.getX()),y=py(e.getY());if(x>=0&&y>=0&&x<meta[0]&&y<meta[1]){NativeCore.begin(core);stroke=true;lastX=x;lastY=y;NativeCore.line(core,x,y,x,y,ink,brush,eraser);refresh();}return true;}
            if(action==MotionEvent.ACTION_MOVE&&stroke&&!multi){for(int i=0;i<e.getHistorySize();i++){int x=px(e.getHistoricalX(i)),y=py(e.getHistoricalY(i));NativeCore.line(core,lastX,lastY,x,y,ink,brush,eraser);lastX=x;lastY=y;}int x=px(e.getX()),y=py(e.getY());NativeCore.line(core,lastX,lastY,x,y,ink,brush,eraser);lastX=x;lastY=y;refresh();}
            if(action==MotionEvent.ACTION_CANCEL){if(stroke)NativeCore.command(core,0,0);stroke=false;multi=false;refresh();}
            if(action==MotionEvent.ACTION_UP){if(stroke){changed();showTool();}stroke=false;multi=false;performClick();}
            return true;
        }catch(Throwable problem){stroke=false;multi=false;error(problem);return true;}}
        @Override public boolean performClick(){super.performClick();return true;}
    }
}
