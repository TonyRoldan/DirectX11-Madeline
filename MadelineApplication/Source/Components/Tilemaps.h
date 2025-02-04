#ifndef TILEMAPS_H
#define TILEMAPS_H

#include "Tiles.h"

namespace MAD
{
	struct Spawnpoint
	{
		GW::MATH::GVECTORF position;
		USHORT otherSceneIndex;
	};

	struct TilemapTile
	{
		USHORT tilesetId;
		USHORT orientationId;

		TilemapTile(USHORT _tilesetId, USHORT _orientationId)
		{
			tilesetId = _tilesetId;
			orientationId = _orientationId;
		}
	};

	struct CompressedTile
	{
		UCHAR tileCount;
		UCHAR tilesetId;
		UCHAR orientationId;

		CompressedTile() 
		{
			tileCount = 0;
			tilesetId = 0;
			orientationId = 0;
		};

		CompressedTile(UCHAR _tileCount, UCHAR _tilesetId, UCHAR _orientationId)
		{
			tileCount = _tileCount;
			tilesetId = _tilesetId;
			orientationId = _orientationId;
		}

		CompressedTile(const TilemapTile& _tile)
		{
			tileCount = 0;
			tilesetId = _tile.tilesetId;
			orientationId = _tile.orientationId;
		}

		bool Equals(const TilemapTile& _tile)
		{
			return tilesetId == _tile.tilesetId && orientationId == _tile.orientationId;
		}
	};

	struct Tilemap
	{
		INT32 originX;
		INT32 originY;
		UINT32 rows;
		UINT32 columns;
		std::vector<USHORT> neighborScenes;
		std::vector<std::vector<TilemapTile>> tiles;
		std::vector<Spawnpoint> spawnpoints;

		Tilemap() 
		{
			originX = 0;
			originY = 0;
			rows = 0;
			columns = 0;
			tiles = {};
		};

		Tilemap(INT32 _originX, INT32 _originY, UINT32 _rows, UINT32 _columns)
		{
			originX = _originX;
			originY = _originY;
			rows = _rows;
			columns = _columns;

			tiles.resize(rows);
			for (UINT32 row = 0; row < rows; row++)
			{
				tiles[row] = std::vector<TilemapTile>(columns, TilemapTile({0, 0}));
			}
		}

		void AddSpawnpoint(GW::MATH::GVECTORF _worldPos, USHORT _otherSceneIndex)
		{
			spawnpoints.push_back({ _worldPos, _otherSceneIndex });
		}

		void AddSpawnpoint(UINT32 _row, UINT32 _col, USHORT _otherSceneIndex)
		{
			Spawnpoint spawnpoint =
			{
				{(float)(originX + _col), (float)(originY + _row)},
				_otherSceneIndex
			};

			spawnpoints.push_back(spawnpoint);
		}

		void RemoveSpawnpoint(USHORT _otherSceneIndex)
		{
			spawnpoints.erase(
				std::remove_if(spawnpoints.begin(), spawnpoints.end(),
					[_otherSceneIndex](const auto& spawnpoint) {
						return spawnpoint.otherSceneIndex == _otherSceneIndex;
					}),
				spawnpoints.end());
		}

		TilemapTile* GetTile(int _row, int _col)
		{
			if (_row >= 0 && _row < rows && _col >= 0 && _col < columns)
				return &tiles[_row][_col];

			return NULL;
		}

		TilemapTile* GetTile(GW::MATH::GVECTORF _worldPos)
		{
			int row = _worldPos.y - originY;
			int col = _worldPos.x - originX;

			if (row >= 0 && row < rows && col >= 0 && col < columns)
				return &tiles[row][col];

			return NULL;
		}

		TilemapTile* GetTile(const Tile& _tile)
		{
			if (_tile.sceneRow >= 0 && _tile.sceneRow < rows && 
				_tile.sceneCol >= 0 && _tile.sceneCol < columns)
				return &tiles[_tile.sceneRow][_tile.sceneCol];

			return NULL;
		}

		GW::MATH::GVECTORF GetMinCorner()
		{
			return { (float)originX, (float)originY };
		}

		GW::MATH::GVECTORF GetMaxCorner()
		{
			return { (float)originX + columns - 1, (float)originY + rows - 1 };
		}

		const Spawnpoint* GetSpawnpointByScene(USHORT _prevSceneIndex)
		{
			for (int i = 0; i < spawnpoints.size(); i++)
			{
				if (spawnpoints[i].otherSceneIndex == _prevSceneIndex)
					return &spawnpoints[i];
			}

			return nullptr;
		}

		static unsigned GetMembersSize()
		{
			return (sizeof(INT32) * 2) + (sizeof(UINT32) * 2);
		}

		bool IsPointInside(GW::MATH::GVECTORF _worldPos)
		{
			_worldPos.x -= originX;
			_worldPos.y -= originY;

			return 
				IsInRange(_worldPos.x, 0, columns) &&
				IsInRange(_worldPos.y, 0, rows);
		}

		bool IsPointInRange(float _range, GW::MATH::GVECTORF _worldPos)
		{
			_worldPos.x -= originX;
			_worldPos.y -= originY;

			return
				IsInRange(_worldPos.x, -_range, columns + _range) &&
				IsInRange(_worldPos.y, -_range, rows + _range);
		}
	};
};

#endif // !TILEMAPS_H
