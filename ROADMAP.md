# Roadmap: Expo Spatial Layer 🗺️

This document outlines the strategic phases of development for the Expo Spatial Layer Engine. Our goal is to provide the most performant, production-ready GIS engine for React Native.

## ✅ Phase 1: Core Engine (Completed)
- JSI/C++ integration with direct memory access.
- Normalized Web Mercator projection logic.
- Basic viewport culling.
- Binary Zero-Copy data transfer via `TypedArrays`.

## ✅ Phase 2: Native Synchronization (Completed)
- Integration with Google Maps via Native `TileOverlay`.
- Native-to-C++ bridge (JNI) for instant camera updates.
- **QuadTree Integration**: $O(\log n)$ spatial indexing for rendering and picking.
- **Agnostic Styling**: Dynamic color mapping from JS to the tile renderer.
- **Result**: Perfect sync with 1M+ points and <0.5ms interaction latency.

---

## 🚀 Phase 3: Production Readiness (Active)

### 1. 🍎 Full iOS Parity
- Implement `MKTileOverlay` in Swift/Objective-C++.
- Consume the same C++ QuadTree instance on iOS.
- Parity for `pointStyles` and `onPointClick` events.

### 2. 🧩 Dynamic Clustering
- Implement $O(n)$ super-clustering in C++.
- Allow visualization of millions of points at low zoom levels without clutter.
- Support for cluster counts and automatic expansion on zoom.

### 3. 🔥 Heatmap Mode
- GPU-accelerated density maps.
- Use point weights to generate Gaussian-blurred heat gradients.

---

## 🛠️ Phase 4: Developer Experience & Ecosystem

### 1. 🌐 Web Support (WASM)
- Port the QuadTree engine to WebAssembly.
- Provide a JS fallback for MapLibre-GL or Leaflet on the web.

### 2. 📂 GeoJSON & Vector Support
- Native GeoJSON parser to avoid JS overhead for standard GIS formats.
- Support for LineString and Polygon rendering.

### 3. 🎨 Advanced Styling API
- Property-based styling (e.g., scale points based on a "magnitude" attribute).
- Transitions and animations for point updates.

---

## Contributing
We are currently focusing on **iOS Parity**. If you have experience with Objective-C++ and MapKit, we'd love your help! Check out [CONTRIBUTING.md](./CONTRIBUTING.md).
