import ExpoModulesCore

public class ExpoSpatialLayerModule: Module {
  public func definition() -> ModuleDefinition {
    Name("ExpoSpatialLayer")

    OnCreate {
      SpatialLayerJSIInstaller.install(self.appContext)
    }

    // Enables the module to be used as a native view.
    View(ExpoSpatialLayerView.self) {
      Prop("nightMode") { (view: ExpoSpatialLayerView, enabled: Bool) in
        view.setNightMode(enabled)
      }

      Prop("center") { (view: ExpoSpatialLayerView, center: [String: Double]) in
        view.setCenter(center)
      }

      Prop("zoom") { (view: ExpoSpatialLayerView, zoom: Double) in
        view.setZoom(zoom)
      }

      Prop("useAutomaticCamera") { (view: ExpoSpatialLayerView, use: Bool) in
        view.setUseAutomaticCamera(use)
      }
    }
  }
}
