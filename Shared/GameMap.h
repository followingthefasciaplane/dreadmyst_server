#pragma once

#include <vector>
#include <string>
#include <stdint.h>
#include <memory>

// Map cell structure for collision/terrain data
struct MapCell
{
    uint8_t terrain = 0;
    uint8_t flags = 0;      // Collision, water, etc.
    int16_t zone = 0;
    int16_t area = 0;
};

// Base map class - client inherits from this
class GameMap
{
public:
    // Layer enum - unscoped so both GameMap::Defines::X and GameMap::X access patterns work
    // Constants for isometric map layout are included in the enum
    enum Defines
    {
        Layer0 = 0,
        Layer1 = 1,
        Layer2 = 2,
        Layer3 = 3,
        NumLayers = 4,
        CellSize = 32,
        MaxMapSize = 1024,
        // Cell dimensions - used as GameMap::Defines::BaseCellWidth etc.
        BaseCellWidth = 64,
        BaseCellHeight = 32,
        TerrainToCellRatio = 4,
    };

    GameMap() = default;
    virtual ~GameMap() = default;

    virtual bool load(const std::string& filepath);

    // Map dimensions
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    int getMapWidth() const { return m_width; }
    int getMapHeight() const { return m_height; }
    int getTerrainWidth() const { return m_width; }
    int getTerrainHeight() const { return m_height; }

    // Map name
    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }

    // Map ID
    int getId() const { return m_id; }
    void setId(int id) { m_id = id; }

    // Cell access - by coordinates
    const MapCell* getCell(int x, int y) const
    {
        if (x < 0 || x >= m_width || y < 0 || y >= m_height)
            return nullptr;
        return &m_cells[y * m_width + x];
    }

    // Cell access - by linear index
    MapCell* getCell(int cellId)
    {
        if (cellId < 0 || cellId >= static_cast<int>(m_cells.size()))
            return nullptr;
        return &m_cells[cellId];
    }

    const MapCell* getCell(int cellId) const
    {
        if (cellId < 0 || cellId >= static_cast<int>(m_cells.size()))
            return nullptr;
        return &m_cells[cellId];
    }

    // Convert cell ID to coordinates and vice versa
    int cellIdToX(int cellId) const { return m_width > 0 ? cellId % m_width : 0; }
    int cellIdToY(int cellId) const { return m_width > 0 ? cellId / m_width : 0; }
    int coordsToCellId(int x, int y) const { return y * m_width + x; }

    bool isWalkable(int x, int y) const;

    // Static isometric coordinate conversion: world position -> screen position
    // This is used by the rendering system for all world-to-screen calculations
    template<typename Vec2>
    static Vec2 computeRawScreenPosition(const Vec2& worldPos)
    {
        // Standard isometric transformation: diamond-shaped grid
        // x_screen = (world_x - world_y) * cell_half_width
        // y_screen = (world_x + world_y) * cell_half_height
        float screenX = (worldPos.x - worldPos.y) * (Defines::BaseCellWidth / 2);
        float screenY = (worldPos.x + worldPos.y) * (Defines::BaseCellHeight / 2);
        return Vec2(screenX, screenY);
    }

    // Virtual methods for derived classes to override
    virtual void startedLoading() {}
    virtual void finishedLoading() {}
    virtual void onFinishedLoadingCells() {}
    virtual void onResize() {}
    virtual void onTerrainTextureLoaded(const int /*terrainId*/, const std::string& /*texture*/) {}
    virtual void onTerrainZoneLoaded(const int /*terrainId*/, const int /*zoneId*/) {}
    virtual void onTerrainAreaLoaded(const int /*terrainId*/, const int /*areaId*/) {}
    virtual void onCellDataLoaded(const int /*cellId*/, const int /*flags*/,
                                   const std::vector<std::shared_ptr<std::string>>& /*textures*/,
                                   const std::vector<float>& /*layerScale*/) {}

protected:
    int m_id = 0;
    int m_width = 0;
    int m_height = 0;
    std::string m_name;
    std::vector<MapCell> m_cells;
};
