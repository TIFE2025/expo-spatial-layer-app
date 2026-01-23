import type { StyleProp, ViewStyle } from 'react-native';

export type OnLoadEventPayload = {
  url: string;
};

export type ExpoSpatialLayerModuleEvents = {
  onChange: (params: ChangeEventPayload) => void;
};

export type ChangeEventPayload = {
  value: string;
};

export type OnPointClickEventPayload = {
  id: number;
  type: number;
  latitude: number;
  longitude: number;
};

export type ExpoSpatialLayerViewProps = {
  url: string;
  onLoad?: (event: { nativeEvent: OnLoadEventPayload }) => void;
  onPointClick?: (event: { nativeEvent: OnPointClickEventPayload }) => void;
  style?: StyleProp<ViewStyle>;
  nightMode?: boolean;
  center?: { latitude: number; longitude: number };
  zoom?: number;
  useAutomaticCamera?: boolean;
  pointStyles?: Record<number, number>;
};
