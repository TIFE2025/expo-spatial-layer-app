import { requireNativeModule, requireNativeViewManager } from 'expo-modules-core';
import React from 'react';
import { ViewProps } from 'react-native';
import { SpatialLayer } from './src/types';

export const getSpatialLayer = (): SpatialLayer | null => (global as any).SpatialLayer || null;

export const ExpoSpatialLayerModule = requireNativeModule('ExpoSpatialLayer');

export * from './src/types';

import { OnPointClickEventPayload } from './src/ExpoSpatialLayer.types';

export type SpatialMapViewProps = {
    nightMode?: boolean;
    center?: { latitude: number; longitude: number };
    zoom?: number;
    useAutomaticCamera?: boolean;
    onPointClick?: (event: { nativeEvent: OnPointClickEventPayload }) => void;
    pointStyles?: Record<number, number>;
} & ViewProps;

const NativeView = requireNativeViewManager('ExpoSpatialLayer');

export function SpatialMapView(props: SpatialMapViewProps) {
    return <NativeView {...props} />;

}
