// Geo2d.h - 2D Geometry utilities for Dreadmyst client
#pragma once

#include <SFML/System/Vector2.hpp>
#include <cmath>

// Geo2d namespace for 2D geometry types used by client
namespace Geo2d
{
    // Extended Vector2 class with additional methods expected by client code
    struct Vector2 : public sf::Vector2f
    {
        // Inherit constructors
        using sf::Vector2f::Vector2f;
        Vector2() : sf::Vector2f(0, 0) {}
        Vector2(float x, float y) : sf::Vector2f(x, y) {}
        Vector2(const sf::Vector2f& v) : sf::Vector2f(v) {}
        Vector2(const sf::Vector2i& v) : sf::Vector2f(static_cast<float>(v.x), static_cast<float>(v.y)) {}

        // In-place modification methods
        void add(const sf::Vector2f& other) { x += other.x; y += other.y; }
        void subtract(const sf::Vector2f& other) { x -= other.x; y -= other.y; }
        void multiply(float scalar) { x *= scalar; y *= scalar; }
        void divide(float scalar) { if (scalar != 0) { x /= scalar; y /= scalar; } }
        void floorSelf() { x = std::floor(x); y = std::floor(y); }
        void ceilSelf() { x = std::ceil(x); y = std::ceil(y); }

        // Length/magnitude
        float length() const { return std::sqrt(x * x + y * y); }
        float lengthSquared() const { return x * x + y * y; }

        // Normalize in-place
        void normalizeSelf()
        {
            float len = length();
            if (len > 0.0001f) { x /= len; y /= len; }
        }

        // Conversion to integer vector
        sf::Vector2i toInt() const { return sf::Vector2i(static_cast<int>(x), static_cast<int>(y)); }

        // Distance to another point
        float getDist(const Vector2& other) const
        {
            float dx = x - other.x;
            float dy = y - other.y;
            return std::sqrt(dx * dx + dy * dy);
        }

        // Check if the vector is effectively zero/null
        bool isNull() const { return x == 0.0f && y == 0.0f; }

        // Reset to zero
        void clear() { x = 0.0f; y = 0.0f; }
    };

    inline float distance(const Vector2& a, const Vector2& b)
    {
        float dx = b.x - a.x;
        float dy = b.y - a.y;
        return std::sqrt(dx * dx + dy * dy);
    }

    inline Vector2 normalize(const Vector2& v)
    {
        float len = std::sqrt(v.x * v.x + v.y * v.y);
        if (len < 0.0001f) return {0, 0};
        return {v.x / len, v.y / len};
    }

    // Integer coordinate distance
    inline float distance2di(int x1, int y1, int x2, int y2)
    {
        float dx = static_cast<float>(x2 - x1);
        float dy = static_cast<float>(y2 - y1);
        return std::sqrt(dx * dx + dy * dy);
    }

    // Float coordinate distance (alias for common usage)
    inline float distance2d(float x1, float y1, float x2, float y2)
    {
        float dx = x2 - x1;
        float dy = y2 - y1;
        return std::sqrt(dx * dx + dy * dy);
    }

    // Floor a vector's components (returns new vector)
    inline Vector2 floor(const Vector2& v)
    {
        return Vector2(std::floor(v.x), std::floor(v.y));
    }

    // Compute linear cell ID from world coordinates
    inline int computeCellId(int worldX, int worldY, int mapWidth)
    {
        if (mapWidth <= 0) return 0;
        return worldY * mapWidth + worldX;
    }

    // Overload for Vector2 input
    inline int computeCellId(const Vector2& pos, int mapWidth)
    {
        return computeCellId(static_cast<int>(pos.x), static_cast<int>(pos.y), mapWidth);
    }

    // Compute cell coordinates from linear cell ID
    inline void computeCellPos(int cellId, int& outX, int& outY, int mapWidth)
    {
        if (mapWidth <= 0) { outX = outY = 0; return; }
        outX = cellId % mapWidth;
        outY = cellId / mapWidth;
    }

    // Move point (x1,y1) towards (x2,y2) by a given distance
    // Returns a new position that is 'distance' units closer to the target
    inline Vector2 extrude(float x1, float y1, float x2, float y2, float dist)
    {
        float dx = x2 - x1;
        float dy = y2 - y1;
        float len = std::sqrt(dx * dx + dy * dy);
        if (len < 0.0001f) return {x1, y1};
        float ratio = dist / len;
        return {x1 + dx * ratio, y1 + dy * ratio};
    }
}

namespace Util
{

struct GeoBox
{
    // Support both naming conventions: left/top/right/bottom and x/y/w/h
    union { float left; float x; };
    union { float top; float y; };
    union { float w; float _width; };
    union { float h; float _height; };

    // Computed right/bottom based on x,y,w,h
    float right() const { return x + w; }
    float bottom() const { return y + h; }

    GeoBox() : x(0), y(0), w(0), h(0) {}
    GeoBox(float x_, float y_, float w_, float h_) : x(x_), y(y_), w(w_), h(h_) {}

    float width() const { return w; }
    float height() const { return h; }

    bool contains(float px, float py) const
    {
        return px >= x && px <= x + w && py >= y && py <= y + h;
    }

    bool contains(const sf::Vector2f& p) const
    {
        return contains(p.x, p.y);
    }

    bool intersects(const GeoBox& other) const
    {
        return !(x > other.x + other.w || x + w < other.x ||
                 y > other.y + other.h || y + h < other.y);
    }
};

// Alias for compatibility with code using GeoBox2d
using GeoBox2d = GeoBox;

inline bool cordsInBox(float px, float py, const GeoBox& box)
{
    return box.contains(px, py);
}

inline bool cordsInBox(const sf::Vector2f& point, const GeoBox& box)
{
    return box.contains(point);
}

// Legacy overloads for code that passes individual components
inline bool cordsInBox(const sf::Vector2i& point, const sf::Vector2i& topLeft, int width, int height)
{
    return point.x >= topLeft.x && point.x < topLeft.x + width &&
           point.y >= topLeft.y && point.y < topLeft.y + height;
}

inline bool cordsInBox(const sf::Vector2f& point, const sf::Vector2f& topLeft, float width, float height)
{
    return point.x >= topLeft.x && point.x < topLeft.x + width &&
           point.y >= topLeft.y && point.y < topLeft.y + height;
}

inline float pointDistance(const sf::Vector2f& a, const sf::Vector2f& b)
{
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    return std::sqrt(dx * dx + dy * dy);
}

inline sf::Vector2f normalize(const sf::Vector2f& v)
{
    float len = std::sqrt(v.x * v.x + v.y * v.y);
    if (len < 0.0001f) return {0, 0};
    return {v.x / len, v.y / len};
}

// Random number helper - only define if not already defined by StringHelpers.h
#ifndef UTIL_IRAND_DEFINED
#define UTIL_IRAND_DEFINED
inline int irand(int min, int max)
{
    return min + (std::rand() % (max - min + 1));
}
#endif

} // namespace Util
