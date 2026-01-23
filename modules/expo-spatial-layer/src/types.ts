export interface SpatialLayer {
    /**
     * Loads data into the spatial engine.
     * @param data Float32Array in format [lat, lon, id, type, ...]
     */
    loadData(data: Float32Array): void;

    /**
     * Updates the camera state for projection.
     * @param lat Center latitude
     * @param lon Center longitude
     * @param zoom Zoom level
     * @param width Viewport width in logical points
     * @param height Viewport height in logical points
     * @param density Screen density
     */
    setCamera(lat: number, lon: number, zoom: number, width: number, height: number, density: number): void;

    /**
     * Returns approximate memory usage of the spatial index in bytes.
     */
    getMemoryUsage(): number;

    /**
     * Calculates the bounding box of current data.
     */
    getDataBounds(): {
        minLat: number;
        maxLat: number;
        minLon: number;
        maxLon: number;
        centerLat: number;
        centerLon: number;
        suggestedZoom: number;
    } | null;
}

declare global {
    var SpatialLayer: SpatialLayer | undefined;
}
