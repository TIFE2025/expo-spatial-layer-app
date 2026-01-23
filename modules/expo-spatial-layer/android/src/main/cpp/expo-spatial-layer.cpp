#include <jni.h>
#include <jsi/jsi.h>
#include <android/log.h>
#include "SpatialLayer.h"

#define LOG_TAG "SpatialLayer"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

using namespace facebook;

// Global pointer for direct JNI access
std::shared_ptr<jsi::SpatialLayer> g_spatialLayer;

extern "C"
JNIEXPORT void JNICALL
Java_expo_modules_spatiallayer_ExpoSpatialLayerModule_nativeInstall(JNIEnv *env, jobject thiz, jlong jsi_runtime_ptr) {
    auto runtime = reinterpret_cast<jsi::Runtime *>(jsi_runtime_ptr);
    if (!runtime) {
        return;
    }

    g_spatialLayer = std::make_shared<jsi::SpatialLayer>();
    auto object = jsi::Object::createFromHostObject(*runtime, g_spatialLayer);

    runtime->global().setProperty(*runtime, "SpatialLayer", std::move(object));
}

extern "C"
JNIEXPORT void JNICALL
Java_expo_modules_spatiallayer_ExpoSpatialLayerModule_nativeUpdateCamera(JNIEnv *env, jobject thiz, jdouble lat, jdouble lon, jdouble zoom, jdouble width, jdouble height, jdouble density) {
    LOGD("nativeUpdateCamera: lat=%f, lon=%f, zoom=%f, width=%f, height=%f, density=%f", lat, lon, zoom, width, height, density);
    if (g_spatialLayer) {
        g_spatialLayer->setCamera(lat, lon, zoom, width, height, density);
    }
}

extern "C"
JNIEXPORT jdoubleArray JNICALL
Java_expo_modules_spatiallayer_ExpoSpatialLayerModule_nativeGetBounds(JNIEnv *env, jobject thiz) {
    if (!g_spatialLayer) {
        return nullptr;
    }
    
    auto bounds = g_spatialLayer->getBounds();
    if (!bounds.hasData) {
        return nullptr;
    }
    
    jdoubleArray result = env->NewDoubleArray(3);
    jdouble fill[3] = {bounds.centerLat, bounds.centerLon, bounds.suggestedZoom};
    env->SetDoubleArrayRegion(result, 0, 3, fill);
    return result;
}

extern "C"
JNIEXPORT jfloatArray JNICALL
Java_expo_modules_spatiallayer_ExpoSpatialLayerModule_nativeGetPointsForTile(JNIEnv *env, jobject thiz, jint tileX, jint tileY, jint zoom) {
    if (!g_spatialLayer) {
        return nullptr;
    }
    
    std::vector<float> points = g_spatialLayer->getPointsForTile(tileX, tileY, zoom);
    
    if (points.empty()) {
        return nullptr;
    }
    
    jfloatArray result = env->NewFloatArray(points.size());
    env->SetFloatArrayRegion(result, 0, points.size(), points.data());
    return result;
}
extern "C"
JNIEXPORT jdoubleArray JNICALL
Java_expo_modules_spatiallayer_ExpoSpatialLayerModule_nativeFindPointAt(JNIEnv *env, jobject thiz, jdouble lat, jdouble lon, jdouble tolerance) {
    if (!g_spatialLayer) {
        return nullptr;
    }
    
    std::vector<double> result = g_spatialLayer->findPointAt(lat, lon, tolerance);
    
    if (result.empty()) {
        return nullptr;
    }
    
    jdoubleArray jResult = env->NewDoubleArray(result.size());
    env->SetDoubleArrayRegion(jResult, 0, result.size(), result.data());
    return jResult;
}

extern "C"
JNIEXPORT void JNICALL
Java_expo_modules_spatiallayer_ExpoSpatialLayerModule_nativeSetStyles(JNIEnv *env, jobject thiz, jintArray types, jintArray colors) {
    if (!g_spatialLayer) return;
    
    jint* pTypes = env->GetIntArrayElements(types, nullptr);
    jint* pColors = env->GetIntArrayElements(colors, nullptr);
    jsize size = env->GetArrayLength(types);
    
    std::vector<std::pair<int, int>> styles;
    for (jsize i = 0; i < size; i++) {
        styles.push_back({(int)pTypes[i], (int)pColors[i]});
    }
    
    g_spatialLayer->setStyles(styles);
    
    env->ReleaseIntArrayElements(types, pTypes, JNI_ABORT);
    env->ReleaseIntArrayElements(colors, pColors, JNI_ABORT);
}
