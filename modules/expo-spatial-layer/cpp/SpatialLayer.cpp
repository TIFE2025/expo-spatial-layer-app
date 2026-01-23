#include "SpatialLayer.h"
#include <iostream>

namespace facebook {
namespace jsi {

SpatialLayer::SpatialLayer() {
}

Value SpatialLayer::get(Runtime& rt, const PropNameID& name) {
    auto propName = name.utf8(rt);

    if (propName == "loadData") {
        return Function::createFromHostFunction(
            rt, name, 1,
            [this](Runtime& rt, const Value& thisVal, const Value* args, size_t count) -> Value {
                if (count > 0) {
                    this->loadData(rt, args[0]);
                }
                return Value::undefined();
            });
    }

    if (propName == "setCamera") {
        return Function::createFromHostFunction(
            rt, name, 6,
            [this](Runtime& rt, const Value& thisVal, const Value* args, size_t count) -> Value {
                if (count >= 6) {
                    this->setCamera(
                        args[0].asNumber(),
                        args[1].asNumber(),
                        args[2].asNumber(),
                        args[3].asNumber(),
                        args[4].asNumber(),
                        args[5].asNumber()
                    );
                }
                return Value::undefined();
            });
    }

    if (propName == "getDataBounds") {
        return Function::createFromHostFunction(
            rt, name, 0,
            [this](Runtime& rt, const Value& thisVal, const Value* args, size_t count) -> Value {
                return this->getDataBounds(rt);
            });
    }

    if (propName == "getMemoryUsage") {
        return Function::createFromHostFunction(
            rt, name, 0,
            [this](Runtime& rt, const Value& thisVal, const Value* args, size_t count) -> Value {
                 std::lock_guard<std::mutex> lock(m_mutex);
                 size_t memory = m_points.capacity() * sizeof(Point);
                 return Value((double)memory);
            });
    }

    if (propName == "findPointAt") {
        return Function::createFromHostFunction(
            rt, name, 3,
            [this](Runtime& rt, const Value& thisVal, const Value* args, size_t count) -> Value {
                if (count >= 3) {
                    auto result = this->findPointAt(
                        args[0].asNumber(),
                        args[1].asNumber(),
                        args[2].asNumber()
                    );
                    if (result.empty()) return Value::null();
                    
                    Object point(rt);
                    point.setProperty(rt, "id", result[0]);
                    point.setProperty(rt, "type", result[1]);
                    point.setProperty(rt, "lat", result[2]);
                    point.setProperty(rt, "lon", result[3]);
                    return point;
                }
                return Value::null();
            });
    }

    if (propName == "setStyles") {
        return Function::createFromHostFunction(
            rt, name, 1,
            [this](Runtime& rt, const Value& thisVal, const Value* args, size_t count) -> Value {
                if (count >= 1 && args[0].isObject()) {
                    auto stylesArray = args[0].asObject(rt).asArray(rt);
                    std::vector<std::pair<int, int>> styles;
                    for (size_t i = 0; i < stylesArray.size(rt); i++) {
                        auto style = stylesArray.getValueAtIndex(rt, i).asObject(rt).asArray(rt);
                        styles.push_back({
                            (int)style.getValueAtIndex(rt, 0).asNumber(),
                            (int)style.getValueAtIndex(rt, 1).asNumber()
                        });
                    }
                    this->setStyles(styles);
                }
                return Value::undefined();
            });
    }

    return Value::undefined();
}

std::vector<PropNameID> SpatialLayer::getPropertyNames(Runtime& rt) {
    std::vector<PropNameID> names;
    names.push_back(PropNameID::forAscii(rt, "loadData"));
    names.push_back(PropNameID::forAscii(rt, "setCamera"));
    names.push_back(PropNameID::forAscii(rt, "getDataBounds"));
    names.push_back(PropNameID::forAscii(rt, "getMemoryUsage"));
    names.push_back(PropNameID::forAscii(rt, "findPointAt"));
    names.push_back(PropNameID::forAscii(rt, "setStyles"));
    return names;
}

void SpatialLayer::loadData(Runtime& rt, const Value& float32Array) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto obj = float32Array.asObject(rt);
    auto buffer = obj.getProperty(rt, "buffer").asObject(rt).getArrayBuffer(rt);
    
    float* data = (float*)buffer.data(rt);
    size_t length = buffer.size(rt) / sizeof(float);
    
    m_points.clear();
    m_points.reserve(length / 4);
    
    // Initialize QuadTree with world bounds [0, 1]
    m_quadTree = std::make_unique<QuadTree>(Rect{0, 0, 1.0, 1.0});

    for (size_t i = 0; i < length; i += 4) {
        if (i + 3 < length) {
            double lat = static_cast<double>(data[i]);
            double lon = static_cast<double>(data[i + 1]);
            
            double normX = (lon + 180.0) / 360.0;
            
            double latRad = lat * M_PI / 180.0;
            double sinLat = std::sin(latRad);
            double y = 0.5 * std::log((1.0 + sinLat) / (1.0 - sinLat));
            double normY = (1.0 - y / M_PI) / 2.0;

            m_points.push_back({
                normX,
                normY,
                data[i + 2], // id
                data[i + 3], // type
                lat,
                lon
            });
            
            // Add reference to QuadTree
            m_quadTree->insert(&m_points.back());
        }
    }
}

void SpatialLayer::setCamera(double lat, double lon, double zoom, double width, double height, double density) {
    {
        std::lock_guard<std::mutex> lock(m_cameraMutex);
        m_camera = {lat, lon, zoom, width, height, density};
    }
}

Value SpatialLayer::getDataBounds(Runtime& rt) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_points.empty()) {
        return Value::null();
    }

    double minLat = 90.0, maxLat = -90.0;
    double minLon = 180.0, maxLon = -180.0;

    for (const auto& p : m_points) {
        if (p.lat < minLat) minLat = p.lat;
        if (p.lat > maxLat) maxLat = p.lat;
        if (p.lon < minLon) minLon = p.lon;
        if (p.lon > maxLon) maxLon = p.lon;
    }

    Object result(rt);
    result.setProperty(rt, "minLat", minLat);
    result.setProperty(rt, "maxLat", maxLat);
    result.setProperty(rt, "minLon", minLon);
    result.setProperty(rt, "maxLon", maxLon);
    result.setProperty(rt, "centerLat", (minLat + maxLat) / 2.0);
    result.setProperty(rt, "centerLon", (minLon + maxLon) / 2.0);

    // Basic zoom estimation: spread of 0.1 deg ~ zoom 12
    double latDelta = maxLat - minLat;
    double lonDelta = maxLon - minLon;
    double maxDelta = std::max(latDelta, lonDelta);
    
    double suggestedZoom = 12.0;
    if (maxDelta > 0) {
        // Simple log-based estimation for suggested zoom
        // Subtract 0.5 to provide some padding around the points
        suggestedZoom = std::floor(std::log2(360.0 / maxDelta) - 0.5);
        if (suggestedZoom > 18) suggestedZoom = 15; // Cap for safety
        if (suggestedZoom < 2) suggestedZoom = 2;
    }
    result.setProperty(rt, "suggestedZoom", suggestedZoom);

    return result;
}

SpatialLayer::Bounds SpatialLayer::getBounds() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_points.empty()) {
        return {0, 0, 0, false};
    }

    double minLat = 90.0, maxLat = -90.0;
    double minLon = 180.0, maxLon = -180.0;

    for (const auto& p : m_points) {
        if (p.lat < minLat) minLat = p.lat;
        if (p.lat > maxLat) maxLat = p.lat;
        if (p.lon < minLon) minLon = p.lon;
        if (p.lon > maxLon) maxLon = p.lon;
    }

    double latDelta = maxLat - minLat;
    double lonDelta = maxLon - minLon;
    double maxDelta = std::max(latDelta, lonDelta);
    
    double suggestedZoom = 12.0;
    if (maxDelta > 0) {
        suggestedZoom = std::floor(std::log2(360.0 / maxDelta) - 0.5);
        if (suggestedZoom > 18) suggestedZoom = 15;
        if (suggestedZoom < 2) suggestedZoom = 2;
    }

    return {
        (minLat + maxLat) / 2.0,
        (minLon + maxLon) / 2.0,
        suggestedZoom,
        true
    };
}

std::vector<float> SpatialLayer::getPointsForTile(int tileX, int tileY, int zoom) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<float> result;
    
    if (m_points.empty() || !m_quadTree) {
        return result;
    }
    
    // Calculate tile bounds in normalized coordinates [0, 1]
    double numTiles = std::pow(2.0, zoom);
    double tileMinX = (double)tileX / numTiles;
    double tileMinY = (double)tileY / numTiles;
    double tileSize = 1.0 / numTiles;
    
    // Small margin to include points exactly on tile edges
    Rect queryRange = {
        tileMinX - 0.0001 * tileSize,
        tileMinY - 0.0001 * tileSize,
        tileSize * 1.0002,
        tileSize * 1.0002
    };
    
    std::vector<const Point*> foundPoints;
    foundPoints.reserve(m_points.size() / numTiles / numTiles + 100); // Heuristic
    m_quadTree->query(queryRange, foundPoints);
    
    result.reserve(foundPoints.size() * 3);
    
    for (const auto* point : foundPoints) {
        // Normalize to [0, 1] within the tile
        float localX = (float)((point->normX - tileMinX) * numTiles);
        float localY = (float)((point->normY - tileMinY) * numTiles);
        result.push_back(localX);
        result.push_back(localY);
        result.push_back(point->type);
    }
    
    return result;
}

std::vector<double> SpatialLayer::findPointAt(double lat, double lon, double tolerance) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_quadTree) return {};

    // 1. Convert lookup point to normalized coordinates
    double normX = (lon + 180.0) / 360.0;
    double latRad = lat * M_PI / 180.0;
    double sinLat = std::sin(latRad);
    double y = 0.5 * std::log((1.0 + sinLat) / (1.0 - sinLat));
    double normY = (1.0 - y / M_PI) / 2.0;

    // 2. Query QuadTree with a small box
    // Tolerance is roughly in degrees, convert to normalized margin
    double margin = tolerance / 360.0;
    Rect queryRange = { normX - margin, normY - margin, margin * 2, margin * 2 };

    std::vector<const Point*> candidates;
    m_quadTree->query(queryRange, candidates);

    if (candidates.empty()) return {};

    // 3. Find the closest point in the results
    const Point* closest = nullptr;
    double minDistSq = tolerance * tolerance; // Using tolerance as max distance threshold

    for (const auto* p : candidates) {
        // Simple Euclidean distance in normalized space is usually enough for local picking
        double dx = p->normX - normX;
        double dy = p->normY - normY;
        double distSq = dx * dx + dy * dy;

        if (distSq < minDistSq) {
            minDistSq = distSq;
            closest = p;
        }
    }

    if (closest) {
        return { (double)closest->id, (double)closest->type, closest->lat, closest->lon };
    }

    return {};
}

void SpatialLayer::setStyles(const std::vector<std::pair<int, int>>& styles) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_pointStyles.clear();
    for (const auto& style : styles) {
        m_pointStyles[style.first] = style.second;
    }
}

} // namespace jsi
} // namespace facebook
