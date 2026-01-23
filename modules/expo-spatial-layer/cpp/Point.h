#pragma once

namespace facebook {
namespace jsi {

struct Point {
    double normX; // Pre-calculated Mercator X [0, 1] - Double for high-zoom precision
    double normY; // Pre-calculated Mercator Y [0, 1] - Double for high-zoom precision
    float id;
    float type;
    double lat; // Keep for bounds
    double lon;
};

} // namespace jsi
} // namespace facebook
