//
// Created by nyiit on 20-4-16.
//
#ifndef _Cpp_com_nyiit_heartbeat_detector_FaceDetector
#define _Cpp_com_nyiit_heartbeat_detector_FaceDetector

#include "com_nyiit_heartbeat_detector_FaceDetector.h"

#include <string>
#include <vector>
#include <opencv2/core/core.hpp>
#include "opencv2/opencv.hpp"
#include "android/log.h"
// ncnn
#include "net.h"

#include "mtcnn.h"
using namespace std;
using namespace cv;
#define TAG "MtcnnFaceDetector"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,TAG,__VA_ARGS__)
static MTCNN *mtcnn;


//sdk是否初始化成功
bool detection_sdk_init_ok = false;
int *faceInfo = nullptr;

#ifdef __cplusplus
  extern "C" {
#endif

void drawRoi(Mat &image, int* faceInfo);
/*
 * Class:     com_nyiit_heartbeat_detector_FaceDetector
 * Method:    FaceDetectionModelInit
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_com_nyiit_heartbeat_detector_FaceDetector_FaceDetectionModelInit
  (JNIEnv *env, jobject instance,  jstring faceDetectionModelPath_)
  {
      LOGD("JNI开始人脸检测模型初始化");
          //如果已初始化则直接返回
          if (detection_sdk_init_ok) {
              //  LOGD("人脸检测模型已经导入");
              return true;
          }
          jboolean tRet = false;
          if (NULL == faceDetectionModelPath_) {
              //   LOGD("导入的人脸检测的目录为空");
              return tRet;
          }

          //获取MTCNN模型的绝对路径的目录（不是/aaa/bbb.bin这样的路径，是/aaa/)
          const char *faceDetectionModelPath = env->GetStringUTFChars(faceDetectionModelPath_, 0);
          if (NULL == faceDetectionModelPath) {
              return tRet;
          }

          string tFaceModelDir = faceDetectionModelPath;
          string tLastChar = tFaceModelDir.substr(tFaceModelDir.length() - 1, 1);
          //LOGD("init, tFaceModelDir last =%s", tLastChar.c_str());
          //目录补齐/
          if ("\\" == tLastChar) {
              tFaceModelDir = tFaceModelDir.substr(0, tFaceModelDir.length() - 1) + "/";
          } else if (tLastChar != "/") {
              tFaceModelDir += "/";
          }
          LOGD("init, tFaceModelDir=%s", tFaceModelDir.c_str());

          //没判断是否正确导入，懒得改了
          mtcnn = new MTCNN(tFaceModelDir);
          mtcnn->SetMinFace(40);
          mtcnn->SetNumThreads(4);
          mtcnn->SetTimeCount(1);

          env->ReleaseStringUTFChars(faceDetectionModelPath_, faceDetectionModelPath);
          detection_sdk_init_ok = true;
          tRet = true;
          return tRet;
  }

  /*
   * Class:     com_nyiit_heartbeat_detector_FaceDetector
   * Method:    FaceDetect
   * Signature: (JIII)[I
   */
  JNIEXPORT void JNICALL Java_com_nyiit_heartbeat_detector_FaceDetector_FaceDetect
    (JNIEnv *env, jobject instance, jlong imgAddr)
    {
    //  LOGD("JNI开始检测人脸");
        if(!detection_sdk_init_ok){
            LOGD("人脸检测MTCNN模型SDK未初始化，直接返回空");
            return;
        }

        //if (）

        Mat& image  = *(Mat*)imgAddr;
        int imageWidth = image.cols;
        int imageHeight = image.rows;
        int imageChannel = image.channels();
        LOGD("数据宽=%d,高=%d,通道=%d",imageWidth,imageHeight,imageChannel);

        //int tImageDateLen = env->GetArrayLength(imageDate_);
        //if(imageChannel == tImageDateLen / imageWidth / imageHeight){
        //    LOGD("数据宽=%d,高=%d,通道=%d",imageWidth,imageHeight,imageChannel);
        //}
        //else{
        //    LOGD("数据长宽高通道不匹配，直接返回空");
        //    return NULL;
        //}

        /*jbyte *imageDate = env->GetByteArrayElements(imageDate_, NULL);
        if (NULL == imageDate){
            LOGD("导入数据为空，直接返回空");
            env->ReleaseByteArrayElements(imageDate_, imageDate, 0);
            return NULL;
        }

        if(imageWidth<20||imageHeight<20){
            LOGD("导入数据的宽和高小于20，直接返回空");
            env->ReleaseByteArrayElements(imageDate_, imageDate, 0);
            return NULL;
        }
        */

        //TODO 通道需测试
        if(3 == imageChannel || 4 == imageChannel){
            //图像通道数只能是3或4；
        }else{
            LOGD("图像通道数只能是3或4，直接返回空");
            //env->ReleaseByteArrayElements(imageDate_, imageDate, 0);
            return;
        }

        //int32_t minFaceSize=40;
        //mtcnn->SetMinFace(minFaceSize);

        /*unsigned char *faceImageCharDate = (unsigned char*)imageDate;*/
        ncnn::Mat ncnn_img;
         /*
        if(imageChannel==3) {
           ncnn_img = ncnn::Mat::from_pixels(faceImageCharDate, ncnn::Mat::PIXEL_BGR2RGB,
                                                        imageWidth, imageHeight);
        }else{
            ncnn_img = ncnn::Mat::from_pixels(faceImageCharDate, ncnn::Mat::PIXEL_RGBA2RGB, imageWidth, imageHeight);
        }
        */
        if(imageChannel==3) {
            ncnn_img = ncnn::Mat::from_pixels(image.data,  ncnn::Mat::PIXEL_RGB, imageWidth, imageHeight);
        } else {
            ncnn_img = ncnn::Mat::from_pixels(image.data,  ncnn::Mat::PIXEL_RGBA2RGB, imageWidth, imageHeight);
        }

        std::vector<Bbox> finalBbox;

        LOGD("开始检测>>>>>>\n");
        mtcnn->detect(ncnn_img, finalBbox);

        int32_t num_face = static_cast<int32_t>(finalBbox.size());
        LOGD("检测到的人脸数目：%d\n", num_face);

        int out_size = 1+num_face*14;
        //  LOGD("内部人脸检测完成,开始导出数据");
        int *faceInfo = new int[out_size];
        faceInfo[0] = num_face;
        for(int i=0;i<num_face;i++){
            faceInfo[14*i+1] = finalBbox[i].x1;//left
            faceInfo[14*i+2] = finalBbox[i].y1;//top
            faceInfo[14*i+3] = finalBbox[i].x2;//right
            faceInfo[14*i+4] = finalBbox[i].y2;//bottom
            for (int j =0;j<10;j++){
                faceInfo[14*i+5+j]=static_cast<int>(finalBbox[i].ppoint[j]);
            }
        }


        drawRoi(image, faceInfo);

        //jintArray tFaceInfo = env->NewIntArray(out_size);
        //env->SetIntArrayRegion(tFaceInfo,0,out_size,faceInfo);
        //  LOGD("内部人脸检测完成,导出数据成功");
        delete[] faceInfo;
        //env->ReleaseByteArrayElements(imageDate_, imageDate, 0);
        //return tFaceInfo;
    }

    void drawRoi(Mat &image, int* faceInfo)
    {
        rectangle(image, Rect(faceInfo[1], faceInfo[2], faceInfo[3] - faceInfo[1], faceInfo[4] - faceInfo[2]), Scalar(0, 255, 0));

        int top = faceInfo[2];
        int eyeLeftX = faceInfo[5];
        int eyeLeftY = faceInfo[10];
        int eyeRightX = faceInfo[6];
        int eyeRightY = faceInfo[11];
        int eyeWidth = eyeRightX - eyeLeftX;
        int eyeCenterY = (eyeRightY + eyeLeftY) / 2;
        int topHeight = eyeCenterY - top;

        rectangle(image, Rect(eyeLeftX + eyeWidth / 5 , eyeCenterY - 0.8 * topHeight, 0.6 * eyeWidth, 0.3 * topHeight), Scalar(0, 255, 0));
    }

  /*
   * Class:     com_nyiit_heartbeat_detector_FaceDetector
   * Method:    MaxFaceDetect
   * Signature: (JIII)[I
   */
  JNIEXPORT void JNICALL Java_com_nyiit_heartbeat_detector_FaceDetector_MaxFaceDetect
    (JNIEnv *env, jobject, jlong imgAddr)
    {

    }

  /*
   * Class:     com_nyiit_heartbeat_detector_FaceDetector
   * Method:    FaceDetectionModelUnInit
   * Signature: ()Z
   */
  JNIEXPORT jboolean JNICALL Java_com_nyiit_heartbeat_detector_FaceDetector_FaceDetectionModelUnInit
    (JNIEnv *env, jobject instance)
    {
        if(!detection_sdk_init_ok){
                LOGD("人脸检测MTCNN模型已经释放过或者未初始化");
                return true;
            }
            jboolean tDetectionUnInit = false;
            delete mtcnn;

            detection_sdk_init_ok=false;
            tDetectionUnInit = true;
            LOGD("人脸检测初始化锁，重新置零");
            return tDetectionUnInit;
    }

  /*
   * Class:     com_nyiit_heartbeat_detector_FaceDetector
   * Method:    SetMinFaceSize
   * Signature: (I)Z
   */
  JNIEXPORT jboolean JNICALL Java_com_nyiit_heartbeat_detector_FaceDetector_SetMinFaceSize
    (JNIEnv *env, jobject instance, jint minSize)
    {
        if(!detection_sdk_init_ok){
                LOGD("人脸检测MTCNN模型SDK未初始化，直接返回");
                return false;
            }

            if(minSize<=20){
                minSize=20;
            }
        mtcnn->SetMinFace(minSize);
        return true;
    }

  /*
   * Class:     com_nyiit_heartbeat_detector_FaceDetector
   * Method:    SetThreadsNumber
   * Signature: (I)Z
   */
  JNIEXPORT jboolean JNICALL Java_com_nyiit_heartbeat_detector_FaceDetector_SetThreadsNumber
    (JNIEnv *env, jobject instance, jint threadsNumber)
    {
         if(!detection_sdk_init_ok){
                 LOGD("人脸检测MTCNN模型SDK未初始化，直接返回");
                 return false;
             }

             if(threadsNumber!=1&&threadsNumber!=2&&threadsNumber!=4&&threadsNumber!=8){
                 LOGD("线程只能设置1，2，4，8");
                 return false;
             }

         mtcnn->SetNumThreads(threadsNumber);
         return  true;
    }

  /*
   * Class:     com_nyiit_heartbeat_detector_FaceDetector
   * Method:    SetTimeCount
   * Signature: (I)Z
   */
  JNIEXPORT jboolean JNICALL Java_com_nyiit_heartbeat_detector_FaceDetector_SetTimeCount
    (JNIEnv *env, jobject instance, jint timeCount)
    {
        if(!detection_sdk_init_ok){
            LOGD("人脸检测MTCNN模型SDK未初始化，直接返回");
            return false;
        }

        mtcnn->SetTimeCount(timeCount);
        return true;
    }

#ifdef __cplusplus
}
#endif
#endif


