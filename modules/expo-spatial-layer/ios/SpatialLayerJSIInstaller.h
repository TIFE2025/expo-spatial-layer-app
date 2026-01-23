@class EXAppContext;

@interface SpatialLayerJSIInstaller : NSObject


+ (void)install:(EXAppContext *)appContext;
+ (void)updateCameraWithLat:(double)lat lon:(double)lon zoom:(double)zoom width:(double)width height:(double)height density:(double)density;
+ (NSDictionary *)getBounds;

@end

