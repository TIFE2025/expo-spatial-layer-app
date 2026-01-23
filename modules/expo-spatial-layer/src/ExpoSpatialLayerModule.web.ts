import { registerWebModule, NativeModule } from 'expo';

import { ChangeEventPayload } from './ExpoSpatialLayer.types';

type ExpoSpatialLayerModuleEvents = {
  onChange: (params: ChangeEventPayload) => void;
}

class ExpoSpatialLayerModule extends NativeModule<ExpoSpatialLayerModuleEvents> {
  PI = Math.PI;
  async setValueAsync(value: string): Promise<void> {
    this.emit('onChange', { value });
  }
  hello() {
    return 'Hello world! 👋';
  }
};

export default registerWebModule(ExpoSpatialLayerModule, 'ExpoSpatialLayerModule');
