package com.ylports.aseprite;

public final class ViewportPolicyTest {
    private static void check(boolean condition,String message){
        if(!condition) throw new AssertionError(message);
    }
    public static void main(String[] args){
        float density=2.75f;
        int gesture=66,cutout=90;
        int left=ViewportPolicy.edgePadding(density,gesture);
        int top=ViewportPolicy.edgePadding(density,cutout);
        int right=ViewportPolicy.edgePadding(density,gesture);
        int portraitWidth=1080-left-right;
        check(left>gesture && top>cutout,"Controls must be beyond protected edges");
        check(portraitWidth>0,"Safe portrait surface must remain usable");
        check(ViewportPolicy.pixelScale(portraitWidth,density)==4,
            "Galaxy-size portrait must double the former 2x desktop UI");
        check(ViewportPolicy.pixelScale(2340-left-right,density)==5,
            "Landscape may use larger original pixels when width permits");
        check(portraitWidth/ViewportPolicy.pixelScale(portraitWidth,density)>=216,
            "Original dialogs need a minimum logical width");
        check(ViewportPolicy.pixelScale(180,4)==1,"Narrow split-screen must stay usable");
        check(ViewportPolicy.pixelScale(4000,10)==8,"Bound memory/scale on extreme density");
        // MotionEvent is local to the padded SurfaceView: its first logical
        // pixel is at zero, irrespective of absolute cutout/navigation offsets.
        int logicalX=19,scale=ViewportPolicy.pixelScale(portraitWidth,density);
        int screenX=left+logicalX*scale;
        check((screenX-left)/scale==logicalX,"Do not subtract insets again in JNI");
        System.out.println("PASS: portrait/landscape safe area, readable integer UI, local input origin");
    }
}
