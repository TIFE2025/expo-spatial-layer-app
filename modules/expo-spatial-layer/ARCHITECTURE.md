# Expo Spatial Layer - Architecture

## Why we switched from Skia to TileOverlay?

### The Original Problem

When rendering points over a map using React Native Skia as an overlay, we faced a fundamental issue: **synchronization latency**.

```
┌─────────────────────────────────────────────────────────────┐
│                    SKIA OVERLAY ARCHITECTURE                 │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│   Google Maps (Native Thread)                               │
│        │                                                    │
│        ▼                                                    │
│   Camera Move Event                                         │
│        │                                                    │
│        ▼                                                    │
│   JS Bridge (async) ──────► Latency ~16-32ms                │
│        │                                                    │
│        ▼                                                    │
│   Reanimated SharedValue                                    │
│        │                                                    │
│        ▼                                                    │
│   Skia Canvas Redraw ─────► "Lagging" points                │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

**Issues encountered:**

1. **Visual Desynchronization**: Points moved with the map for 1-2 frames before being repositioned.
2. **FPS Drops**: Projecting 10k+ points on the JS thread (even with worklets) caused FPS drops.
3. **Complexity**: We tried various solutions (native events, Choreographer, C++ projection) without total success.

### The Solution: TileOverlay

TileOverlay is a native Google Maps API that allows rendering custom tiles **as part of the map itself**. This means perfect synchronization.

```
┌─────────────────────────────────────────────────────────────┐
│                  TILEOVERLAY ARCHITECTURE                    │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│   Google Maps (Native Thread)                               │
│        │                                                    │
│        ├──► Renders Base Tiles                              │
│        │                                                    │
│        └──► Renders Custom Tiles (points) ◄── SAME CYCLE    │
│                    │                                        │
│                    ▼                                        │
│              SpatialTileProvider                            │
│                    │                                        │
│                    ▼                                        │
│              C++ getPointsForTile()                         │
│                    │                                        │
│                    ▼                                        │
│              Bitmap 512x512 PNG                             │
│                                                             │
│   ✅ Perfect Sync - Same render pipeline                    │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

**Advantages:**

| Aspect | Skia Overlay | TileOverlay |
|---------|--------------|-------------|
| Synchronization | ❌ 1-2 frames latency | ✅ Perfect |
| Performance | ⚠️ Depends on JS thread | ✅ Native |
| Scalability | ⚠️ 10k points = lag | ✅ 200k+ points |
| Cache | ❌ Redraws every frame | ✅ Cached tiles |
| Complexity | High (bridge, worklets) | Medium |

---

## Module Architecture

### Overview

```
expo-spatial-layer/
├── index.tsx                    # Public export
├── src/
│   ├── ExpoSpatialLayer.types.ts    # TypeScript types
│   ├── ExpoSpatialLayerModule.ts    # JS → Native Interface
│   └── ExpoSpatialLayerView.tsx     # React Component
├── cpp/
│   ├── SpatialLayer.cpp             # Core C++ engine
│   └── SpatialLayer.h
├── android/
│   └── src/main/
│       ├── java/.../
│       │   ├── ExpoSpatialLayerModule.kt   # Expo Module
│       │   ├── ExpoSpatialLayerView.kt     # MapView + TileOverlay
│       │   └── SpatialTileProvider.kt      # Tile generator
│       └── cpp/
│           └── expo-spatial-layer.cpp      # JNI Bridge
└── ios/
    ├── ExpoSpatialLayerModule.swift
    └── ExpoSpatialLayerView.swift          # MapKit implementation
```

### Layers

```
┌─────────────────────────────────────────────────────────────┐
│                      JAVASCRIPT LAYER                       │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│   spatial-test.tsx                                          │
│        │                                                    │
│        ├── getSpatialLayer().loadData(data) // Load data    │
│        │                                                    │
│        └── <SpatialMapView />               // Render map   │
│                                                             │
└──────────────────────────┬──────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────┐
│                    EXPO MODULE LAYER                        │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│   ExpoSpatialLayerModule.kt                                 │
│        │                                                    │
│        ├── nativeInstall()  → Exposes SpatialLayer host object │
│        └── native methods   → Direct C++ calls via JNI      │
│                                                             │
│   ExpoSpatialLayerView.kt                                   │
│        │                                                    │
│        ├── GoogleMap setup                                  │
│        ├── TileOverlay with SpatialTileProvider             │
│        └── Camera listeners                                 │
│                                                             │
└──────────────────────────┬──────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────┐
│                       C++ LAYER (JSI)                       │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│   SpatialLayer.cpp                                          │
│        │                                                    │
│        ├── QuadTree m_quadTree        // O(log n) indexing  │
│        │                                                    │
│        ├── loadData(data)                                   │
│        │       └── Normalized Web Mercator projection       │
│        │                                                    │
│        ├── getPointsForTile(tileX, tileY, zoom)             │
│        │       ├── Calculates tile bounds                    │
│        │       ├── QuadTree query                           │
│        │       └── Returns [x, y, type] normalized          │
│        │                                                    │
│        └── findPointAt(lat, lon, tolerance)                 │
│                └── Radial query for picking                 │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

## Core Components

### 1. Spatial Engine (C++ / JSI)
The heart of the module resides in C++ to ensure raw performance and memory safety.
- **QuadTree Indexing**: We implemented a QuadTree for $O(\log n)$ spatial search. When loading data, points are projected to Web Mercator and inserted into the tree.
- **Performance**: Capable of indexing 200k+ points in ~50ms and performing tile queries in <2ms.

### 2. Agnostic Rendering (Dynamic Styling)
The engine is data-agnostic.
- **Metadata**: Each point has an associated `type`.
- **Dynamic Configuration**: JS passes a `Record<number, number>` map (Type -> Hex Color) via the `pointStyles` prop.
- **Pipeline**: The native `SpatialTileProvider` retrieves colors dynamically during bitmap drawing, allowing map appearance changes without reloading data.

### 3. Interactivity (JSI Collision Detection)
- **Zero-Latency Picking**: When clicking the map, `ExpoSpatialLayerView` sends coordinates to C++ via JNI.
- **Spatial Query**: The QuadTree performs a radial query (with tolerance) to find the ID of the clicked object.
- **Event Flow**: The result is sent back to JS via `onPointClick`, triggering the callback with all selected point metadata.

---

## Redesigned Data Flow

### 1. Data Loading (Binary Format)
To avoid JSON parsing overhead, we use a binary buffer (`Float32Array[lat, lon, id, type]`).
```
JS Float32Array
       │
       ▼ (JSI Direct Access)
C++ loadData()
       │
       ├── Project to Web Mercator [0,1]
       └── Insert into QuadTree
```

---

## Performance (200k points)

| Operation | Previous Time | Current Time (QuadTree) |
|----------|----------------|------------------------|
| loadData() | ~150ms (JSON) | ~40ms (Binary) |
| Tile Query (Zoom 10) | ~15ms (Linear) | <1ms |
| Point Click (Picking) | N/A | <0.5ms |

---

## 🗺️ Future Roadmap

- [x] Implement Spatial Index (Quadtree)
- [x] Hit-testing for point interaction (onPointClick)
- [x] Agnostic dynamic styling (pointStyles)
- [ ] **Full iOS Parity**: Porting the view layer to `MKTileOverlay`.
- [ ] **Dynamic Clustering**: $O(n)$ implementation for low-zoom legibility.
- [ ] **Configurable Sizing**: Radius control per point type from JS.
- [ ] **Heatmap Mode**: Density-based rendering using GPU shaders.
- [ ] **Web Support**: QuadTree integration via WASM.

---

## References

- [Google Maps TileOverlay](https://developers.google.com/maps/documentation/android-sdk/tileoverlay)
- [Web Mercator Projection](https://en.wikipedia.org/wiki/Web_Mercator_projection)
- [Expo Modules API](https://docs.expo.dev/modules/overview/)
- [QuadTree Wiki](https://en.wikipedia.org/wiki/Quadtree)
