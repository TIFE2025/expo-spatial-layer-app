package expo.modules.spatiallayer

import android.content.Context
import android.graphics.Color
import android.util.Log
import android.view.Choreographer
import android.view.ViewGroup.LayoutParams
import com.facebook.react.bridge.LifecycleEventListener
import com.facebook.react.bridge.ReactContext
import com.google.android.gms.maps.CameraUpdateFactory
import com.google.android.gms.maps.GoogleMap
import com.google.android.gms.maps.MapView
import com.google.android.gms.maps.model.LatLng
import com.google.android.gms.maps.model.MapStyleOptions
import com.google.android.gms.maps.model.TileOverlay
import com.google.android.gms.maps.model.TileOverlayOptions
import expo.modules.kotlin.AppContext
import expo.modules.kotlin.viewevent.ViewEventDelegate
import expo.modules.kotlin.views.ExpoView

private const val TAG = "ExpoSpatialLayerView"

class ExpoSpatialLayerView(context: Context, appContext: AppContext) : ExpoView(context, appContext), LifecycleEventListener {
  private val spatialModule: ExpoSpatialLayerModule?
    get() = appContext.registry.getModule("ExpoSpatialLayer") as? ExpoSpatialLayerModule

  private val onPointClick by ViewEventDelegate<Map<String, Any>>(this, null)

  private val mapView = MapView(context)
  private var googleMap: GoogleMap? = null
  private var isMapReady = false
  private var nightMode = false
  private var useAutomaticCamera = false
  private var hasAutoFitted = false
  private var pendingCenter: LatLng? = null
  private var pendingZoom: Float? = null
  private val handler = android.os.Handler(android.os.Looper.getMainLooper())
  
  // Tile overlay for rendering points
  private var tileOverlay: TileOverlay? = null
  private val tileProvider = SpatialTileProvider(
    tileSize = 512,
    pointRadius = 8f,
    pointColor = Color.CYAN
  )
  
  // Throttle camera updates
  private var lastCameraUpdateTime = 0L
  private var pendingCameraUpdate = false
  private val choreographer = Choreographer.getInstance()
  private val frameCallback = object : Choreographer.FrameCallback {
    override fun doFrame(frameTimeNanos: Long) {
      if (pendingCameraUpdate) {
        pendingCameraUpdate = false
        doUpdateNativeCamera()
      }
    }
  }

  init {
    val params = LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT)
    addView(mapView, params)
    
    // Set initial black background to prevent white flash
    setBackgroundColor(0xFF000000.toInt())
    mapView.setBackgroundColor(0xFF000000.toInt())

    // Forward lifecycle events immediately
    mapView.onCreate(null)
    mapView.onStart()

    mapView.getMapAsync { map ->
      googleMap = map
      isMapReady = true
      setupMap()
    }

    // Register for React Native lifecycle events
    (appContext.reactContext as? ReactContext)?.addLifecycleEventListener(this)
  }

  private fun setupMap() {
    val map = googleMap ?: return
    
    // Default settings
    map.uiSettings.isCompassEnabled = false
    map.uiSettings.isMyLocationButtonEnabled = false
    map.uiSettings.isMapToolbarEnabled = false
    map.uiSettings.isTiltGesturesEnabled = false // Tilt breaks the 2D overlay alignment
    map.uiSettings.isRotateGesturesEnabled = false // Rotation is harder to sync in early stages

    // Performance optimizations: Disable features that render 3D models or extra layers
    map.isBuildingsEnabled = false
    map.isIndoorEnabled = false
    map.isTrafficEnabled = false
    
    applyMapStyle()
    
    // Assign module to tile provider
    spatialModule?.let { tileProvider.setModule(it) }

    // Add tile overlay for rendering points - this ensures perfect sync with the map
    tileOverlay = map.addTileOverlay(
      TileOverlayOptions()
        .tileProvider(tileProvider)
        .transparency(0f)
        .fadeIn(false) // Disable fade animation to prevent flicker
        .zIndex(100f) // Above map tiles
    )

    // Apply pending camera updates
    pendingCenter?.let { 
      map.moveCamera(CameraUpdateFactory.newLatLng(it))
      pendingCenter = null
    }
    pendingZoom?.let {
      map.moveCamera(CameraUpdateFactory.zoomTo(it))
      pendingZoom = null
    }

    if (useAutomaticCamera && !hasAutoFitted) {
      tryAutomaticCameraFit()
    }

    map.setOnCameraMoveListener {
      updateNativeCamera()
    }
    
    map.setOnCameraIdleListener {
      updateNativeCamera()
    }

    map.setOnMapClickListener { latLng ->
      // Standard tolerance for picking (roughly 20-30 meters in degrees)
      val tolerance = 0.0002 
      val pointData = spatialModule?.findPointAt(latLng.latitude, latLng.longitude, tolerance)
      
      if (pointData != null && pointData.size >= 4) {
        // Emit event to JS
        // pointData is [id, type, lat, lon]
        Log.d(TAG, "Point clicked: ID=${pointData[0]}")
        onPointClick(mapOf(
          "id" to pointData[0].toInt(),
          "type" to pointData[1].toInt(),
          "latitude" to pointData[2],
          "longitude" to pointData[3]
        ))
      }
    }

    // Critical: Listen for layout changes to update camera with correct dimensions
    // This fixes the "Points don't show immediately" issue
    addOnLayoutChangeListener { _, left, top, right, bottom, oldLeft, oldTop, oldRight, oldBottom ->
      if (left != oldLeft || top != oldTop || right != oldRight || bottom != oldBottom) {
         updateNativeCamera()
      }
    }

    // Trigger initial update
    updateNativeCamera()
  }

  fun setPointStyles(styles: Map<String, Int>) {
    val intStyles = styles.entries.associate { it.key.toInt() to it.value }
    val types = intStyles.keys.toIntArray()
    val colors = intStyles.values.toIntArray()
    spatialModule?.setStyles(types, colors)
    tileProvider.setPointStyles(intStyles)
    handler.post {
      // Clear cache and refresh if needed
      tileOverlay?.clearTileCache()
    }
  }

  fun setNightMode(enabled: Boolean) {
    nightMode = enabled
    // Prevent white flash by setting background color
    mapView.setBackgroundColor(if (enabled) 0xFF000000.toInt() else 0xFFFFFFFF.toInt())
    applyMapStyle()
  }

  fun setCenter(lat: Double, lon: Double) {
    val map = googleMap
    if (map != null) {
      map.moveCamera(CameraUpdateFactory.newLatLng(LatLng(lat, lon)))
      updateNativeCamera()
    } else {
      pendingCenter = LatLng(lat, lon)
    }
  }

  fun setZoom(zoom: Float) {
    val map = googleMap
    if (map != null) {
      map.moveCamera(CameraUpdateFactory.zoomTo(zoom))
      updateNativeCamera()
    } else {
      pendingZoom = zoom
    }
  }

  fun setUseAutomaticCamera(use: Boolean) {
    this.useAutomaticCamera = use
    if (use && !hasAutoFitted) {
      startAutoFitRetry()
    }
  }

  private var autoFitRunnable: Runnable? = null
  private var autoFitRetryCount = 0
  
  private fun startAutoFitRetry() {
    autoFitRunnable?.let { handler.removeCallbacks(it) }
    autoFitRetryCount = 0
    
    // Try immediately first
    tryAutomaticCameraFit()
    
    // Then retry every 100ms for up to 3 seconds
    autoFitRunnable = object : Runnable {
      override fun run() {
        autoFitRetryCount++
        
        if (hasAutoFitted || autoFitRetryCount > 30) {
          return
        }
        
        tryAutomaticCameraFit()
        handler.postDelayed(this, 100)
      }
    }
    handler.postDelayed(autoFitRunnable!!, 100)
  }

  private fun tryAutomaticCameraFit() {
    val map = googleMap ?: return
    if (!useAutomaticCamera) return
    
    val bounds = spatialModule?.getBounds()
    Log.d(TAG, "tryAutomaticCameraFit: bounds=$bounds, hasAutoFitted=$hasAutoFitted")
    
    if (bounds != null && bounds.size >= 3) {
      val lat = bounds[0]
      val lon = bounds[1]
      val zoom = bounds[2].toFloat()
      
      Log.d(TAG, "Setting camera to lat=$lat, lon=$lon, zoom=$zoom")
      
      map.moveCamera(CameraUpdateFactory.newLatLngZoom(LatLng(lat, lon), zoom))
      hasAutoFitted = true
      updateNativeCamera()
    }
  }

  private fun applyMapStyle() {
    val map = googleMap ?: return
    if (nightMode) {
      val nightStyleJson = """
        [
          { "elementType": "geometry", "stylers": [ { "color": "#242f3e" } ] },
          { "elementType": "labels.text.fill", "stylers": [ { "color": "#746855" } ] },
          { "elementType": "labels.text.stroke", "stylers": [ { "color": "#242f3e" } ] },
          { "featureType": "administrative.locality", "elementType": "labels.text.fill", "stylers": [ { "color": "#d59563" } ] },
          { "featureType": "poi", "elementType": "labels.text.fill", "stylers": [ { "color": "#d59563" } ] },
          { "featureType": "poi.park", "elementType": "geometry", "stylers": [ { "color": "#263c3f" } ] },
          { "featureType": "poi.park", "elementType": "labels.text.fill", "stylers": [ { "color": "#6b9a76" } ] },
          { "featureType": "road", "elementType": "geometry", "stylers": [ { "color": "#38414e" } ] },
          { "featureType": "road", "elementType": "geometry.stroke", "stylers": [ { "color": "#212a37" } ] },
          { "featureType": "road", "elementType": "labels.text.fill", "stylers": [ { "color": "#9ca5b3" } ] },
          { "featureType": "road.highway", "elementType": "geometry", "stylers": [ { "color": "#746855" } ] },
          { "featureType": "road.highway", "elementType": "geometry.stroke", "stylers": [ { "color": "#1f2835" } ] },
          { "featureType": "road.highway", "elementType": "labels.text.fill", "stylers": [ { "color": "#f3d19c" } ] },
          { "featureType": "water", "elementType": "geometry", "stylers": [ { "color": "#17263c" } ] },
          { "featureType": "water", "elementType": "labels.text.fill", "stylers": [ { "color": "#515c6d" } ] },
          { "featureType": "water", "elementType": "labels.text.stroke", "stylers": [ { "color": "#17263c" } ] }
        ]
      """.trimIndent()
      map.setMapStyle(MapStyleOptions(nightStyleJson))
    } else {
      map.setMapStyle(null)
    }
  }

  private fun updateNativeCamera() {
    // Schedule update on next frame to batch multiple camera events
    if (!pendingCameraUpdate) {
      pendingCameraUpdate = true
      choreographer.postFrameCallback(frameCallback)
    }
  }
  
  private fun doUpdateNativeCamera() {
    val map = googleMap ?: return
    val position = map.cameraPosition
    
    // Safety check: ensure we have valid dimensions
    if (width <= 0 || height <= 0) {
      return
    }

    val density = resources.displayMetrics.density.toDouble()
    // Convert pixels to logical points (width/height are in pixels on Android)
    val widthInPoints = width.toDouble() / density
    val heightInPoints = height.toDouble() / density
    
    // Call the module to update C++ layer
    spatialModule?.updateCameraExperimental(
      position.target.latitude,
      position.target.longitude,
      position.zoom.toDouble(),
      widthInPoints,
      heightInPoints,
      density
    )
  }
  
  /**
   * Force refresh of tile overlay to show updated points.
   * Call this after data is loaded or when points change.
   */
  fun refreshTiles() {
    tileOverlay?.clearTileCache()
  }

  override fun onHostResume() {
    mapView.onResume()
  }

  override fun onHostPause() {
    mapView.onPause()
  }

  override fun onHostDestroy() {
    mapView.onDestroy()
    (appContext.reactContext as? ReactContext)?.removeLifecycleEventListener(this)
  }
  
  // Note: ExpoView deals with layout, but we match parent so it should be fine.
}
