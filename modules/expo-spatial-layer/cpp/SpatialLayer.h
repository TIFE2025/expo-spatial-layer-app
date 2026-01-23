#pragma once

#include <jsi/jsi.h>
#include <vector>
#include <mutex>
#include <atomic>
#include <cmath>
#include <unordered_map>
#include "QuadTree.h"

namespace facebook {
namespace jsi {

class SpatialLayer : public HostObject {
public:
    SpatialLayer();
    virtual ~SpatialLayer() = default;

    // HostObject methods
    Value get(Runtime& rt, const PropNameID& name) override;
    std::vector<PropNameID> getPropertyNames(Runtime& rt) override;

    // Internal methods (exposed to JS)
    void loadData(Runtime& rt, const Value& float32Array);
    void setCamera(double lat, double lon, double zoom, double width, double height, double density);
    Value getDataBounds(Runtime& rt);
    
    // Tile-based rendering - returns points normalized to [0,1] within the tile
    std::vector<float> getPointsForTile(int tileX, int tileY, int zoom);

    // Collision detection - returns {id, type, dist} or empty vector
    std::vector<double> findPointAt(double lat, double lon, double tolerance);

    // Style management
    void setStyles(const std::vector<std::pair<int, int>>& styles);

    // Direct access for native side
    struct Bounds {
        double centerLat;
        double centerLon;
        double suggestedZoom;
        bool hasData;
    };
    Bounds getBounds() const;

private:
    struct Camera {
        double lat;
        double lon;
        double zoom;
        double width; 
        double height;
        double density; 
    };

    std::vector<Point> m_points;
    std::unique_ptr<QuadTree> m_quadTree;
    Camera m_camera;

    mutable std::mutex m_mutex;       // Protects m_points
    mutable std::mutex m_cameraMutex; // Protects m_camera
    std::unordered_map<int, int> m_pointStyles; // type -> hex color
};

} // namespace jsi
} // namespace facebook
