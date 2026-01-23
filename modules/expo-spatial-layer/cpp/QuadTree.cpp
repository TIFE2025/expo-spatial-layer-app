#include "QuadTree.h"
#include "SpatialLayer.h"

namespace facebook {
namespace jsi {

bool QuadTreeNode::insert(const Point* point) {
    if (!m_bounds.contains(point->normX, point->normY)) {
        return false;
    }

    if (m_points.size() < CAPACITY && !m_divided) {
        m_points.push_back(point);
        return true;
    }

    if (!m_divided) {
        subdivide();
    }

    if (m_children[0]->insert(point)) return true;
    if (m_children[1]->insert(point)) return true;
    if (m_children[2]->insert(point)) return true;
    if (m_children[3]->insert(point)) return true;

    return false;
}

void QuadTreeNode::subdivide() {
    double x = m_bounds.x;
    double y = m_bounds.y;
    double w = m_bounds.w / 2.0;
    double h = m_bounds.h / 2.0;

    m_children[0] = std::make_unique<QuadTreeNode>(Rect{x + w, y, w, h});     // NE
    m_children[1] = std::make_unique<QuadTreeNode>(Rect{x, y, w, h});         // NW
    m_children[2] = std::make_unique<QuadTreeNode>(Rect{x, y + h, w, h});     // SW
    m_children[3] = std::make_unique<QuadTreeNode>(Rect{x + w, y + h, w, h}); // SE

    m_divided = true;

    // Distribute existing points to children
    for (const auto* p : m_points) {
        for (auto& child : m_children) {
            if (child->insert(p)) break;
        }
    }
    m_points.clear();
}

void QuadTreeNode::query(const Rect& range, std::vector<const Point*>& found) const {
    if (!m_bounds.intersects(range)) {
        return;
    }

    if (m_divided) {
        for (const auto& child : m_children) {
            child->query(range, found);
        }
    } else {
        for (const auto* p : m_points) {
            if (range.contains(p->normX, p->normY)) {
                found.push_back(p);
            }
        }
    }
}

} // namespace jsi
} // namespace facebook
