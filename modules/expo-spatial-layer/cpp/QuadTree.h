#pragma once

#include <vector>
#include <memory>
#include <array>
#include "Point.h"

namespace facebook {
namespace jsi {

struct Rect {
    double x, y, w, h;

    bool contains(double px, double py) const {
        return px >= x && px < x + w && py >= y && py < y + h;
    }

    bool intersects(const Rect& other) const {
        return !(other.x > x + w || 
                 other.x + other.w < x || 
                 other.y > y + h ||
                 other.y + other.h < y);
    }
};

class QuadTreeNode {
public:
    static const int CAPACITY = 32;

    QuadTreeNode(Rect bounds) : m_bounds(bounds), m_divided(false) {}

    bool insert(const Point* point);
    void query(const Rect& range, std::vector<const Point*>& found) const;

private:
    void subdivide();

    Rect m_bounds;
    std::vector<const Point*> m_points;
    bool m_divided;
    
    // Using unique_ptr for automatic memory management
    std::array<std::unique_ptr<QuadTreeNode>, 4> m_children;
};

class QuadTree {
public:
    QuadTree(Rect bounds) : m_root(std::make_unique<QuadTreeNode>(bounds)) {}

    void insert(const Point* point) {
        m_root->insert(point);
    }

    void query(const Rect& range, std::vector<const Point*>& found) const {
        m_root->query(range, found);
    }

    void clear(Rect bounds) {
        m_root = std::make_unique<QuadTreeNode>(bounds);
    }

private:
    std::unique_ptr<QuadTreeNode> m_root;
};

} // namespace jsi
} // namespace facebook
