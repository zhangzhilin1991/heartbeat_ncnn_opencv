package com.nyiit.heartbeat;

import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Display;

import com.nyiit.heartbeat.detector.FaceDetector;
import com.nyiit.heartbeat.utils.FileUtil;

import org.opencv.android.BaseLoaderCallback;
import org.opencv.android.CameraBridgeViewBase;
import org.opencv.android.JavaCameraView;
import org.opencv.android.LoaderCallbackInterface;
import org.opencv.android.OpenCVLoader;
import org.opencv.core.Core;
import org.opencv.core.CvType;
import org.opencv.core.Mat;
import org.opencv.core.Point;
import org.opencv.core.Scalar;
import org.opencv.imgproc.Imgproc;

import java.io.File;
import java.io.IOException;
import java.util.Collections;
import java.util.List;

import static org.opencv.core.Core.FONT_HERSHEY_SIMPLEX;

public class CameraPreviewActivity extends CameraActivity implements CameraBridgeViewBase.CvCameraViewListener2 {

    public static final String TAG = CameraPreviewActivity.class.getName();

    // Used to load the 'native-lib' library on application startup.
    Mat mRgb;
    Mat mRgbT;
    Mat mGray;
    Mat mGrayT;
    JavaCameraView mOpenCvCameraView;

    FaceDetector faceDetector = new FaceDetector();

    private BaseLoaderCallback mLoaderCallback = new BaseLoaderCallback(this) {
        @Override
        public void onManagerConnected(int status) {
            switch (status) {
                case LoaderCallbackInterface.SUCCESS:
                    //mOpenCvCameraView.setDisplayOrientation(90);
                    //System.loadLibrary("heartbeat");
                    System.loadLibrary("mtcnnfacedetector");
                    initDetector();

                    mOpenCvCameraView.enableView();
                    break;
                default:
                    super.onManagerConnected(status);
            }
        }
    };

    void initDetector() {
        //拷贝模型到sk卡
        try {
            FileUtil.copyBigDataToSD(this,"det1.bin");
            FileUtil.copyBigDataToSD(this, "det2.bin");
            FileUtil.copyBigDataToSD(this,"det3.bin");
            FileUtil.copyBigDataToSD(this,"det1.param");
            FileUtil.copyBigDataToSD(this,"det2.param");
            FileUtil.copyBigDataToSD(this,"det3.param");
        } catch (IOException e) {
            e.printStackTrace();
        }
        //模型初始化
        File sdDir = Environment.getExternalStorageDirectory();//获取跟目录
        String sdPath = sdDir.toString() + "/mtcnn/";
        faceDetector.FaceDetectionModelInit(sdPath);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_camerapreview);

        //initDetector();

        Display display = getWindow().getWindowManager().getDefaultDisplay();
        DisplayMetrics displayMetrics = new DisplayMetrics();
        display.getMetrics(displayMetrics);


        int width = displayMetrics.widthPixels;//宽度
        int height = displayMetrics.heightPixels;//高度

        Log.d(TAG, "display width: " + width + ", height: " + height);

        mOpenCvCameraView = findViewById(R.id.hb_activity_surface_view);
        mOpenCvCameraView.setVisibility(CameraBridgeViewBase.VISIBLE);
        mOpenCvCameraView.setMaxFrameSize(480, 640); //landscape
        mOpenCvCameraView.setCvCameraViewListener(this);
        // Example of a call to a native method
        //TextView tv = findViewById(R.id.sample_text);
        checkPermission();
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (OpenCVLoader.initDebug()) {
            Log.d(TAG, "OpenCV library found inside package. Using it!");
            mLoaderCallback.onManagerConnected(LoaderCallbackInterface.SUCCESS);
        } else {
            Log.d(TAG, "OpenCV library not found inside package. exit!");
            this.finish();
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (mOpenCvCameraView != null) {
            mOpenCvCameraView.disableView();
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        faceDetector.FaceDetectionModelUnInit();
        mOpenCvCameraView.disableView();
    }

    @Override
    protected List<? extends CameraBridgeViewBase> getCameraViewList() {
        return Collections.singletonList(mOpenCvCameraView);
    }

    @Override
    public void onCameraViewStarted(int width, int height) {
        mRgb = new Mat(height, width, CvType.CV_8UC3);
        mRgbT = new Mat(height, width, CvType.CV_8UC3);
       // mGray = new Mat(height, width, CvType.CV_8UC1);
      //  mGrayT = new Mat();
    }

    @Override
    public void onCameraViewStopped() {
        mRgb.release();
        //mGray.release();
        //mGrayT.release();
    }

    @Override
    public Mat onCameraFrame(CameraBridgeViewBase.CvCameraViewFrame inputFrame) {

        mRgb = inputFrame.rgba();
        if (mRgbT != null) {
            mRgbT.release();
        }
        mRgbT = mRgb.t();
        Core.flip(mRgbT, mRgbT, 1);
        Imgproc.resize(mRgbT, mRgbT, mRgb.size());

        //mGray = inputFrame.gray();
        //if (mGrayT != null) {
       //     mGrayT.release();
       // }
       // mGrayT = mGray.t();
       // Core.flip(mGrayT, mGrayT, 1);
       // Imgproc.resize(mGrayT, mGrayT, mGray.size());

        //HeartBeatDetector.detectHeartBeat(mRgbT, mGrayT);
        faceDetector.FaceDetect(mRgbT.nativeObj);

        int fps = fps();
        Imgproc.putText(mRgbT, "fps: " + fps, new Point(30,60),FONT_HERSHEY_SIMPLEX, 1, new Scalar(255,0,0,255), 2);
        //mRgba.release();
        return mRgbT;
    }

    int fps = 0;
    long lastTime = System.currentTimeMillis(); // ms
    int frameCount = 0;

    int fps() {
        ++frameCount;

        long curTime = System.currentTimeMillis();
        if (curTime - lastTime > 1000) // 取固定时间间隔为1秒
        {
            fps = frameCount;
            frameCount = 0;
            lastTime = curTime;
        }
        return fps;

    }

    private int MY_PERMISSIONS_REQUEST_READ_EXTERNAL_STORAGE = 0;

    void checkPermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            if (checkSelfPermission(Manifest.permission.READ_EXTERNAL_STORAGE)
                    != PackageManager.PERMISSION_GRANTED) {

                // Should we show an explanation?
                if (shouldShowRequestPermissionRationale(
                        Manifest.permission.READ_EXTERNAL_STORAGE)) {
                    // Explain to the user why we need to read the contacts
                }

                requestPermissions(new String[]{Manifest.permission.READ_EXTERNAL_STORAGE},
                        MY_PERMISSIONS_REQUEST_READ_EXTERNAL_STORAGE);

                // MY_PERMISSIONS_REQUEST_READ_EXTERNAL_STORAGE is an
                // app-defined int constant that should be quite unique

                return;
            }
        }
    }
}
