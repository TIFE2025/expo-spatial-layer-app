import { NativeModule, requireNativeModule } from 'expo';

import { ExpoSpatialLayerModuleEvents } from './ExpoSpatialLayer.types';

declare class ExpoSpatialLayerModule extends NativeModule<ExpoSpatialLayerModuleEvents> {
}

// This call loads the native module object from the JSI.
export default requireNativeModule<ExpoSpatialLayerModule>('ExpoSpatialLayer');
