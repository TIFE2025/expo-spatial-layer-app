# Contributing to Expo Spatial Layer

Thank you for your interest in improving the Expo Spatial Layer Engine! This is a high-performance native module, and we welcome contributions in JavaScript, TypeScript, Kotlin, Swift, and C++.

## How to Help

### 1. iOS Parity
The core engine is C++, but the view layer needs implementation for iOS using `MKTileOverlay`. This is our current top priority.

### 2. Feature Requests
If you have ideas for new GIS features (clustering, heatmaps, GeoJSON support), please open an issue to discuss.

## Technical Architecture

The module uses a **Three-Layer Architecture**:
1. **JavaScript**: The public API and state management.
2. **Native (Kotlin/Swift)**: Map lifecycle and Bitmap/Tile rendering.
3. **Engine (C++)**: QuadTree indexing and spatial projections via JSI.

Please read the [**Architecture Guide**](./modules/expo-spatial-layer/ARCHITECTURE.md) before making changes to the C++ core.

## Development Workflow

1. Fork the repository.
2. Create a feature branch.
3. Run the demo app to verify your changes:
   ```bash
   npx expo run:android
   ```
4. Submit a Pull Request with a clear description of your changes.

---

Happy Hacking! 🌏🚀
