import { requireNativeView } from 'expo';
import * as React from 'react';

import { ExpoSpatialLayerViewProps } from './ExpoSpatialLayer.types';

const NativeView: React.ComponentType<ExpoSpatialLayerViewProps> =
  requireNativeView('ExpoSpatialLayer');

export default function ExpoSpatialLayerView(props: ExpoSpatialLayerViewProps) {
  return <NativeView {...props} />;
}
