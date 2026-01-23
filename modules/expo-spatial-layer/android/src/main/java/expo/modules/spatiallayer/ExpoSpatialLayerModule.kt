package expo.modules.spatiallayer

import expo.modules.kotlin.modules.Module
import expo.modules.kotlin.modules.ModuleDefinition
import com.facebook.react.bridge.ReactContext

class ExpoSpatialLayerModule : Module() {
  override fun definition() = ModuleDefinition {
    Name("ExpoSpatialLayer")

    OnCreate {
      val reactContext = appContext.reactContext as? ReactContext
      reactContext?.let {
        val jsiPtr = it.javaScriptContextHolder?.get()
        if (jsiPtr != null && jsiPtr != 0L) {
          nativeInstall(jsiPtr)
        }
      }
    }

    View(ExpoSpatialLayerView::class) {
      Events("onPointClick")

      Prop("nightMode") { view: ExpoSpatialLayerView, enabled: Boolean ->
        view.setNightMode(enabled)
      }

      Prop("center") { view: ExpoSpatialLayerView, center: Map<String, Double> ->
        val lat = center["latitude"] ?: 0.0
        val lon = center["longitude"] ?: 0.0
        view.setCenter(lat, lon)
      }

      Prop("zoom") { view: ExpoSpatialLayerView, zoom: Float ->
        view.setZoom(zoom)
      }

      Prop("useAutomaticCamera") { view: ExpoSpatialLayerView, use: Boolean ->
        view.setUseAutomaticCamera(use)
      }

      Prop("pointStyles") { view: ExpoSpatialLayerView, styles: Map<String, Int> ->
        view.setPointStyles(styles)
      }
    }
  }

  fun updateCameraExperimental(lat: Double, lon: Double, zoom: Double, width: Double, height: Double, density: Double) {
    nativeUpdateCamera(lat, lon, zoom, width, height, density)
  }

  fun getBounds(): DoubleArray? {
    return nativeGetBounds()
  }

  fun getPointsForTile(tileX: Int, tileY: Int, zoom: Int): FloatArray? {
    return nativeGetPointsForTile(tileX, tileY, zoom)
  }

  fun findPointAt(lat: Double, lon: Double, tolerance: Double): DoubleArray? {
    return nativeFindPointAt(lat, lon, tolerance)
  }

  fun setStyles(types: IntArray, colors: IntArray) {
    nativeSetStyles(types, colors)
  }

  private external fun nativeInstall(jsiPtr: Long)
  private external fun nativeUpdateCamera(lat: Double, lon: Double, zoom: Double, width: Double, height: Double, density: Double)
  private external fun nativeGetBounds(): DoubleArray?
  private external fun nativeGetPointsForTile(tileX: Int, tileY: Int, zoom: Int): FloatArray?
  private external fun nativeFindPointAt(lat: Double, lon: Double, tolerance: Double): DoubleArray?
  private external fun nativeSetStyles(types: IntArray, colors: IntArray)

  companion object {
    init {
      System.loadLibrary("expo-spatial-layer")
    }
  }
}
