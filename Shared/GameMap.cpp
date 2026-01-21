// GameMap.cpp - Base map class implementation
// Handles loading/parsing of .map files and basic collision checking

#include "GameMap.h"
#include <fstream>
#include <cstring>

// StlBuffer is used for serialization
#if PLATFORM_WINDOWS || defined(_WIN32)
#include "StlBuffer.h"
#else
// For Linux, use a simple buffer reader
class SimpleBuffer {
public:
    SimpleBuffer(const char* data, size_t size) : m_data(data), m_size(size), m_pos(0) {}

    template<typename T>
    T read() {
        if (m_pos + sizeof(T) > m_size) return T{};
        T val;
        std::memcpy(&val, m_data + m_pos, sizeof(T));
        m_pos += sizeof(T);
        return val;
    }

    std::string readString() {
        int len = read<int>();
        if (len <= 0 || m_pos + static_cast<size_t>(len) > m_size) return "";
        std::string str(m_data + m_pos, len);
        m_pos += len;
        return str;
    }

    bool eof() const { return m_pos >= m_size; }

private:
    const char* m_data;
    size_t m_size;
    size_t m_pos;
};
#endif

bool GameMap::load(const std::string& filepath)
{
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open())
        return false;

    startedLoading();

    // Read entire file into memory
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(static_cast<size_t>(size));
    if (!file.read(buffer.data(), size))
    {
        return false;
    }

#if PLATFORM_WINDOWS || defined(_WIN32)
    StlBuffer stlBuffer;
    stlBuffer.write(buffer.data(), buffer.size());
    stlBuffer.resetRead();

    // Read map size
    int mapSize = 0;
    stlBuffer >> mapSize;
#else
    SimpleBuffer stlBuffer(buffer.data(), buffer.size());
    int mapSize = stlBuffer.read<int>();
#endif

    if (mapSize <= 0 || mapSize > Defines::MaxMapSize)
    {
        return false;
    }

    m_width = mapSize;
    m_height = mapSize;

    // Resize cells array
    m_cells.resize(static_cast<size_t>(m_width * m_height));

    // Read texture dictionary
#if PLATFORM_WINDOWS || defined(_WIN32)
    int numTextures = 0;
    stlBuffer >> numTextures;
#else
    int numTextures = stlBuffer.read<int>();
#endif

    std::vector<std::string> textureDictionary;
    textureDictionary.reserve(static_cast<size_t>(numTextures));

    for (int i = 0; i < numTextures; ++i)
    {
#if PLATFORM_WINDOWS || defined(_WIN32)
        std::string texName;
        stlBuffer >> texName;
        textureDictionary.push_back(texName);
#else
        textureDictionary.push_back(stlBuffer.readString());
#endif
    }

    // Notify resize
    onResize();

    // Read cells with data
#if PLATFORM_WINDOWS || defined(_WIN32)
    int numCells = 0;
    stlBuffer >> numCells;
#else
    int numCells = stlBuffer.read<int>();
#endif

    for (int i = 0; i < numCells; ++i)
    {
#if PLATFORM_WINDOWS || defined(_WIN32)
        int cellId = 0;
        stlBuffer >> cellId;

        unsigned char flags = 0;
        stlBuffer >> flags;
#else
        int cellId = stlBuffer.read<int>();
        unsigned char flags = stlBuffer.read<unsigned char>();
#endif

        if (cellId < 0 || cellId >= static_cast<int>(m_cells.size()))
            continue;

        m_cells[cellId].flags = flags;

        // Read layer data
        std::vector<std::shared_ptr<std::string>> layerTextures;
        std::vector<float> layerScales;
        layerTextures.resize(Defines::NumLayers);
        layerScales.resize(Defines::NumLayers, 1.0f);

        for (int layer = 0; layer < Defines::NumLayers; ++layer)
        {
#if PLATFORM_WINDOWS || defined(_WIN32)
            bool hasTexture = false;
            stlBuffer >> hasTexture;
#else
            bool hasTexture = stlBuffer.read<bool>();
#endif

            if (hasTexture)
            {
#if PLATFORM_WINDOWS || defined(_WIN32)
                int texIndex = 0;
                stlBuffer >> texIndex;

                float scale = 1.0f;
                stlBuffer >> scale;
#else
                int texIndex = stlBuffer.read<int>();
                float scale = stlBuffer.read<float>();
#endif

                if (texIndex >= 0 && texIndex < static_cast<int>(textureDictionary.size()))
                {
                    layerTextures[layer] = std::make_shared<std::string>(textureDictionary[texIndex]);
                    layerScales[layer] = scale;
                }
            }
        }

        onCellDataLoaded(cellId, flags, layerTextures, layerScales);
    }

    onFinishedLoadingCells();

    // Read terrain data
#if PLATFORM_WINDOWS || defined(_WIN32)
    int numTerrainTextures = 0;
    stlBuffer >> numTerrainTextures;
#else
    int numTerrainTextures = stlBuffer.read<int>();
#endif

    std::vector<std::string> terrainDictionary;
    terrainDictionary.reserve(static_cast<size_t>(numTerrainTextures));

    for (int i = 0; i < numTerrainTextures; ++i)
    {
#if PLATFORM_WINDOWS || defined(_WIN32)
        std::string texName;
        stlBuffer >> texName;
        terrainDictionary.push_back(texName);
#else
        terrainDictionary.push_back(stlBuffer.readString());
#endif
    }

#if PLATFORM_WINDOWS || defined(_WIN32)
    int numTerrains = 0;
    stlBuffer >> numTerrains;
#else
    int numTerrains = stlBuffer.read<int>();
#endif

    for (int i = 0; i < numTerrains; ++i)
    {
#if PLATFORM_WINDOWS || defined(_WIN32)
        int terrainId = 0;
        stlBuffer >> terrainId;

        int texIndex = 0;
        stlBuffer >> texIndex;
#else
        int terrainId = stlBuffer.read<int>();
        int texIndex = stlBuffer.read<int>();
#endif

        if (texIndex >= 0 && texIndex < static_cast<int>(terrainDictionary.size()))
        {
            onTerrainTextureLoaded(terrainId, terrainDictionary[texIndex]);
        }
    }

    // Read zone/area data if present
#if PLATFORM_WINDOWS || defined(_WIN32)
    if (!stlBuffer.isEof())
    {
        int numZones = 0;
        stlBuffer >> numZones;

        for (int i = 0; i < numZones; ++i)
        {
            int terrainId = 0, zoneId = 0;
            stlBuffer >> terrainId >> zoneId;
            onTerrainZoneLoaded(terrainId, zoneId);
        }
    }

    if (!stlBuffer.isEof())
    {
        int numAreas = 0;
        stlBuffer >> numAreas;

        for (int i = 0; i < numAreas; ++i)
        {
            int terrainId = 0, areaId = 0;
            stlBuffer >> terrainId >> areaId;
            onTerrainAreaLoaded(terrainId, areaId);
        }
    }
#else
    if (!stlBuffer.eof())
    {
        int numZones = stlBuffer.read<int>();

        for (int i = 0; i < numZones; ++i)
        {
            int terrainId = stlBuffer.read<int>();
            int zoneId = stlBuffer.read<int>();
            onTerrainZoneLoaded(terrainId, zoneId);
        }
    }

    if (!stlBuffer.eof())
    {
        int numAreas = stlBuffer.read<int>();

        for (int i = 0; i < numAreas; ++i)
        {
            int terrainId = stlBuffer.read<int>();
            int areaId = stlBuffer.read<int>();
            onTerrainAreaLoaded(terrainId, areaId);
        }
    }
#endif

    finishedLoading();
    return true;
}

bool GameMap::isWalkable(int x, int y) const
{
    const MapCell* cell = getCell(x, y);
    if (cell == nullptr)
        return false;

    // Flag 0x01 typically means blocked/unwalkable
    return (cell->flags & 0x01) == 0;
}
