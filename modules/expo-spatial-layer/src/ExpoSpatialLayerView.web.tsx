import * as React from 'react';

import { ExpoSpatialLayerViewProps } from './ExpoSpatialLayer.types';

export default function ExpoSpatialLayerView(props: ExpoSpatialLayerViewProps) {
  return (
    <div>
      <iframe
        style={{ flex: 1 }}
        src={props.url}
        onLoad={() => props.onLoad({ nativeEvent: { url: props.url } })}
      />
    </div>
  );
}
