import ExpoModulesCore
import MapKit

class ExpoSpatialLayerView: ExpoView, MKMapViewDelegate {
  let mapView = MKMapView()
  var useAutomaticCamera = false
  var hasAutoFitted = false
  var autoFitTimer: Timer?
  var autoFitRetryCount = 0

  required init(appContext: AppContext? = nil) {
    super.init(appContext: appContext)
    clipsToBounds = true
    
    mapView.delegate = self
    mapView.showsUserLocation = false
    mapView.showsCompass = false
    mapView.showsScale = false
    
    // Default to dark modeish style if possible or standard
    // MapKit customization is limited compared to Google Maps styles JSON, 
    // but we can set map type.
    mapView.mapType = .standard
    
    addSubview(mapView)
  }
  
  deinit {
    autoFitTimer?.invalidate()
  }

  func setNightMode(_ enabled: Bool) {
    if #available(iOS 13.0, *) {
      mapView.overrideUserInterfaceStyle = enabled ? .dark : .light
    }
    // Also use mutedStandard for a darker feel when in light mode? 
    // Usually nightMode means dark theme.
  }

  func setCenter(_ center: [String: Double]) {
    guard let lat = center["latitude"], let lon = center["longitude"] else { return }
    let coordinate = CLLocationCoordinate2D(latitude: lat, longitude: lon)
    mapView.setCenter(coordinate, animated: false)
    updateNativeCamera()
  }

  func setZoom(_ zoom: Double) {
    // Standard MKMapView behavior: longitudeDelta is used to calculate zoom level.
    // latDelta is usually adjusted by MKMapView to fit the view's aspect ratio.
    let lonDelta = 360.0 / pow(2.0, zoom)
    let span = MKCoordinateSpan(latitudeDelta: lonDelta, longitudeDelta: lonDelta)
    let region = MKCoordinateRegion(center: mapView.centerCoordinate, span: span)
    mapView.setRegion(region, animated: false)
    updateNativeCamera()
  }

  func setUseAutomaticCamera(_ use: Bool) {
    self.useAutomaticCamera = use
    if use && !hasAutoFitted {
      // Start retry timer to handle data loading delay
      startAutoFitRetry()
    }
  }
  
  func startAutoFitRetry() {
    autoFitTimer?.invalidate()
    autoFitRetryCount = 0
    
    // Try immediately first
    tryAutomaticCameraFit()
    
    // Then retry every 100ms for up to 3 seconds
    autoFitTimer = Timer.scheduledTimer(withTimeInterval: 0.1, repeats: true) { [weak self] timer in
      guard let self = self else { 
        timer.invalidate()
        return 
      }
      
      self.autoFitRetryCount += 1
      
      if self.hasAutoFitted || self.autoFitRetryCount > 30 {
        timer.invalidate()
        self.autoFitTimer = nil
        return
      }
      
      self.tryAutomaticCameraFit()
    }
  }

  func tryAutomaticCameraFit() {
    guard useAutomaticCamera else { return }
    print("[ExpoSpatialLayerView] tryAutomaticCameraFit called, hasAutoFitted=\(hasAutoFitted)")
    
    if let bounds = SpatialLayerJSIInstaller.getBounds() {
      print("[ExpoSpatialLayerView] Got bounds: \(bounds)")
      if let lat = bounds["centerLat"] as? Double,
         let lon = bounds["centerLon"] as? Double,
         let zoom = bounds["suggestedZoom"] as? Double {
        
        print("[ExpoSpatialLayerView] Setting camera to lat=\(lat), lon=\(lon), zoom=\(zoom)")
        
        let coordinate = CLLocationCoordinate2D(latitude: lat, longitude: lon)
        let lonDelta = 360.0 / pow(2.0, zoom)
        let span = MKCoordinateSpan(latitudeDelta: lonDelta, longitudeDelta: lonDelta)
        let region = MKCoordinateRegion(center: coordinate, span: span)
        
        mapView.setRegion(region, animated: false)
        hasAutoFitted = true
        updateNativeCamera()
      }
    } else {
      print("[ExpoSpatialLayerView] No bounds available yet")
    }
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    mapView.frame = bounds
    // Update camera when layout changes (important for initial render)
    updateNativeCamera()
    
    // Retry auto-fit if it hasn't happened yet (data might have loaded after initial attempt)
    if useAutomaticCamera && !hasAutoFitted {
      tryAutomaticCameraFit()
    }
  }
  
  // MKMapViewDelegate methods
  func mapViewDidChangeVisibleRegion(_ mapView: MKMapView) {
    updateNativeCamera()
  }
  
  func updateNativeCamera() {
    let region = mapView.region
    let center = region.center
    
    // Calculate zoom level from span
    // Approximation for Web Mercator zoom level
    // log2(360 / longitudeDelta)
    let zoomLevel = log2(360.0 / region.span.longitudeDelta)
    
    // Bounds in points
    let width = self.bounds.width
    let height = self.bounds.height
    let scale = UIScreen.main.scale
    
    // Safety check for bounds
    if (width > 0 && height > 0) {
      SpatialLayerJSIInstaller.updateCamera(
        withLat: center.latitude,
        lon: center.longitude,
        zoom: zoomLevel,
        width: Double(width),
        height: Double(height),
        density: Double(scale)
      )
    }
  }
}
