package org.hololive.caddin;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.PixelFormat;
import android.os.Build;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;

public class CaddinOverlayService extends Service implements View.OnTouchListener, Runnable {
    static {
        System.loadLibrary("caddin");
    }

    private WindowManager windowManager;
    private View overlayView;
    private WindowManager.LayoutParams params;
    private Bitmap petBitmap;
    private Handler handler;
    private int initialX;
    private int initialY;
    private float initialTouchX;
    private float initialTouchY;

    private static final int PET_SIZE_DP = 100;
    private int petSizePx;

    public static native void nativeInit();
    public static native void nativeStep(float dt);
    public static native void nativeRender(Bitmap bitmap);
    public static native void nativeTouch(float x, float y, int action);

    public static class PetView extends View {
        private final Bitmap bitmap;

        public PetView(Context context, Bitmap bmp) {
            super(context);
            this.bitmap = bmp;
        }

        @Override
        protected void onDraw(Canvas canvas) {
            super.onDraw(canvas);
            if (bitmap != null) {
                canvas.drawBitmap(bitmap, 0, 0, null);
            }
        }
    }

    @Override
    public void onCreate() {
        super.onCreate();
        nativeInit();

        handler = new Handler(Looper.getMainLooper());
        windowManager = (WindowManager) getSystemService(WINDOW_SERVICE);
        float density = getResources().getDisplayMetrics().density;
        petSizePx = (int) (PET_SIZE_DP * density);

        petBitmap = Bitmap.createBitmap(petSizePx, petSizePx, Bitmap.Config.ARGB_8888);

        int layoutType = Build.VERSION.SDK_INT >= Build.VERSION_CODES.O
                ? WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY
                : WindowManager.LayoutParams.TYPE_PHONE;

        params = new WindowManager.LayoutParams(
                petSizePx,
                petSizePx,
                layoutType,
                WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE,
                PixelFormat.TRANSLUCENT
        );

        params.gravity = Gravity.TOP | Gravity.START;
        params.x = 50;
        params.y = 150;

        overlayView = new PetView(this, petBitmap);
        overlayView.setOnTouchListener(this);

        windowManager.addView(overlayView, params);
        handler.post(this);
    }

    @Override
    public boolean onTouch(View v, MotionEvent event) {
        nativeTouch(event.getX(), event.getY(), event.getAction());
        switch (event.getAction()) {
            case MotionEvent.ACTION_DOWN:
                initialX = params.x;
                initialY = params.y;
                initialTouchX = event.getRawX();
                initialTouchY = event.getRawY();
                return true;

            case MotionEvent.ACTION_MOVE:
                params.x = initialX + (int) (event.getRawX() - initialTouchX);
                params.y = initialY + (int) (event.getRawY() - initialTouchY);
                windowManager.updateViewLayout(overlayView, params);
                return true;

            case MotionEvent.ACTION_UP:
                int screenWidth = getResources().getDisplayMetrics().widthPixels;
                if (params.x + petSizePx / 2 < screenWidth / 2) {
                    params.x = 10;
                } else {
                    params.x = screenWidth - petSizePx - 10;
                }
                windowManager.updateViewLayout(overlayView, params);
                return true;
        }
        return false;
    }

    @Override
    public void run() {
        nativeStep(0.033f);
        nativeRender(petBitmap);
        if (overlayView != null) {
            overlayView.invalidate();
        }
        if (handler != null) {
            handler.postDelayed(this, 33);
        }
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (handler != null) {
            handler.removeCallbacks(this);
        }
        if (overlayView != null && windowManager != null) {
            windowManager.removeView(overlayView);
        }
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }
}
