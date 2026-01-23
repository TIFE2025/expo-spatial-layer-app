import { Stack } from 'expo-router';
import React, { useEffect, useState } from 'react';
import { Platform, StyleSheet, Text, TouchableOpacity, View } from 'react-native';

import { getSpatialLayer, SpatialMapView } from '../modules/expo-spatial-layer';
// @ts-ignore
import { Asset } from 'expo-asset';

export default function SpatialTestScreen() {
    const [isLoaded, setIsLoaded] = useState(false);
    const [retryCount, setRetryCount] = useState(0);
    const [totalPoints, setTotalPoints] = useState(0);
    const [memoryUsage, setMemoryUsage] = useState(0);
    const [selectedPoint, setSelectedPoint] = useState<any>(null);

    useEffect(() => {
        const spatialLayer = getSpatialLayer() as any;
        if (!spatialLayer) {
            if (retryCount < 10) {
                const timer = setTimeout(() => setRetryCount(prev => prev + 1), 100);
                return () => clearTimeout(timer);
            }
            return;
        }

        async function loadBinaryData() {
            try {
                console.log("Loading binary asset...");
                const asset = Asset.fromModule(require('../assets/taxi-data.bin'));
                await asset.downloadAsync();

                if (!asset.localUri) throw new Error("Could not get local URI");

                console.log(`Fetching binary data from ${asset.localUri}...`);
                const response = await fetch(asset.localUri);
                const buffer = await response.arrayBuffer();
                const pointsArray = new Float32Array(buffer);

                console.log(`Loading ${pointsArray.length / 4} points into C++...`);
                spatialLayer.loadData(pointsArray);

                setTotalPoints(pointsArray.length / 4);
                if (spatialLayer.getMemoryUsage) {
                    setMemoryUsage(spatialLayer.getMemoryUsage() / 1024 / 1024);
                }
                setIsLoaded(true);
            } catch (err) {
                console.error("Failed to load binary data:", err);
            }
        }

        loadBinaryData();
    }, [retryCount]);

    const handlePointClick = (event: any) => {
        const point = event.nativeEvent;
        console.log("Point clicked!", point);
        setSelectedPoint(point);
    };

    if (!isLoaded) return null;

    return (
        <View style={styles.container}>
            <Stack.Screen options={{ headerShown: false }} />
            <SpatialMapView
                style={StyleSheet.absoluteFill}
                nightMode={true}
                useAutomaticCamera={true}
                onPointClick={handlePointClick}
                pointStyles={{
                    1: 0xFF00FFFF, // Cyan (Type 1)
                    2: 0xFFFF8C00, // DarkOrange (Type 2)
                    3: 0xFFADFF2F, // GreenYellow (Type 3)
                }}
            />

            <View style={styles.debugOverlay} pointerEvents="none">
                <Text style={styles.debugText}>Total Points: {totalPoints.toLocaleString()}</Text>
                <Text style={styles.debugText}>Memory: {memoryUsage.toFixed(2)} MB</Text>
                <Text style={styles.debugText}>Format: Binary (Zero-Copy)</Text>
            </View>

            {selectedPoint && (
                <View style={[styles.overlay, { top: 200, left: 20, borderLeftWidth: 4, borderLeftColor: 'lime', paddingRight: 35 }]}>
                    <TouchableOpacity
                        onPress={() => setSelectedPoint(null)}
                        style={styles.closeButton}
                    >
                        <Text style={{ color: 'white', fontSize: 20, fontWeight: 'bold' }}>×</Text>
                    </TouchableOpacity>
                    <Text style={{ color: 'white', fontWeight: 'bold', marginBottom: 6 }}>SELECTED POINT</Text>
                    <Text style={styles.infoText}>ID: {selectedPoint.id}</Text>
                    <Text style={styles.infoText}>Type: {selectedPoint.type}</Text>
                    <Text style={styles.infoText}>Lat: {selectedPoint.latitude.toFixed(5)}</Text>
                    <Text style={styles.infoText}>Lon: {selectedPoint.longitude.toFixed(5)}</Text>
                    <TouchableOpacity
                        style={{ marginTop: 8, backgroundColor: 'rgba(255,255,255,0.1)', padding: 4, borderRadius: 4, alignItems: 'center' }}
                        onPress={() => alert(`Point ${selectedPoint.id} selected!`)}
                    >
                        <Text style={{ color: 'lime', fontSize: 10 }}>ACTION</Text>
                    </TouchableOpacity>
                </View>
            )}

            <View style={styles.overlay}>
                <Text style={{ color: 'lime', fontSize: 10 }}>● Tile Overlay (Perfect Sync)</Text>
            </View>
        </View>
    );
}

const styles = StyleSheet.create({
    container: {
        flex: 1,
        backgroundColor: '#000',
    },
    debugOverlay: {
        position: 'absolute',
        top: 50,
        left: 20,
        backgroundColor: 'rgba(0,0,0,0.85)',
        padding: 12,
        borderRadius: 8,
        zIndex: 10,
        borderWidth: 1,
        borderColor: 'rgba(255,255,255,0.1)',
    },
    debugText: {
        color: 'white',
        fontSize: 13,
        fontWeight: 'bold',
        fontFamily: Platform.OS === 'ios' ? 'Menlo' : 'monospace',
    },
    overlay: {
        position: 'absolute',
        top: 150,
        left: 20,
        backgroundColor: 'rgba(0,0,0,0.85)',
        padding: 12,
        borderRadius: 8,
        borderWidth: 1,
        borderColor: 'rgba(255,255,255,0.1)',
    },
    infoText: {
        color: '#aaa',
        fontSize: 12,
        fontFamily: Platform.OS === 'ios' ? 'Menlo' : 'monospace',
        marginBottom: 2,
    },
    closeButton: {
        position: 'absolute',
        top: 5,
        right: 8,
        width: 30,
        height: 30,
        alignItems: 'center',
        justifyContent: 'center',
    }
});
