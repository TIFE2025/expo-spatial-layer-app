#import "SpatialLayerJSIInstaller.h"
#import "../cpp/SpatialLayer.h"
#import <ExpoModulesCore/EXAppContext.h>
#import <ExpoModulesCore/EXJavaScriptRuntime.h>

// Global pointer for JSI access
static std::shared_ptr<facebook::jsi::SpatialLayer> g_spatialLayer;

@implementation SpatialLayerJSIInstaller

+ (void)install:(EXAppContext *)appContext {
  facebook::jsi::Runtime *runtime = [appContext.runtime get];
  if (runtime == nullptr) {
    return;
  }

  g_spatialLayer = std::make_shared<facebook::jsi::SpatialLayer>();
  auto object = facebook::jsi::Object::createFromHostObject(*runtime, g_spatialLayer);
  
  runtime->global().setProperty(*runtime, "SpatialLayer", std::move(object));
}

+ (void)updateCameraWithLat:(double)lat lon:(double)lon zoom:(double)zoom width:(double)width height:(double)height density:(double)density {
  if (g_spatialLayer) {
    g_spatialLayer->setCamera(lat, lon, zoom, width, height, density);
  }
}

+ (NSDictionary *)getBounds {
  if (!g_spatialLayer) return nil;
  
  auto bounds = g_spatialLayer->getBounds();
  if (!bounds.hasData) return nil;
  
  return @{
    @"centerLat": @(bounds.centerLat),
    @"centerLon": @(bounds.centerLon),
    @"suggestedZoom": @(bounds.suggestedZoom)
  };
}

@end
