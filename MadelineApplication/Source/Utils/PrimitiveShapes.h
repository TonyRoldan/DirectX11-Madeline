#pragma once
#include <vector>

struct Vector3
{
	float x, y, z;
};

struct Vertex
{
	Vector3 pos, uvw, nrm;
};

struct Cube
{
	std::vector< GW::MATH::GVECTORF> verts;
	std::vector<unsigned> indices;

	Cube() : verts({
		{ -0.5f, 0.5f, -0.5f, 1.0f },
		{ -0.5f, 0.5f, 0.5f, 1.0f },
		{ 0.5f, 0.5f, -0.5f, 1.0f },
		{ 0.5f, 0.5f, 0.5f, 1.0f },
		{ 0.5f, -0.5f, -0.5f, 1.0f },
		{ 0.5f, -0.5f, 0.5f, 1.0f },
		{ -0.5f, -0.5f, 0.5f, 1.0f },
		{ -0.5f, -0.5f, -0.5f, 1.0f }
		}), indices({ 0,1,2,2,1,3,2,3,
			5,2,4,5,1,3,5,5,6,1,0,1,6,
			0,7,6,0,7,4,0,2,4,7,6,5,7,5,4 })
	{}
};

struct Quad
{
	std::vector<Vertex> verts;
	std::vector<unsigned> indices;

	Quad() : verts({
	{ {-1.0f,1.0f,0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f } },
	{ {1.0f,1.0f,0.0f}, { 1.0f, 0.0f, 1.0f} ,{0.0f,  0.0f, 0.0f} },
	{ {-1.0f,-1.0f,0.0f}, {0.0f, 1.0f, 1.0f}, {0.0f,  0.0f, 0.0f} },
	{ {1.0f,-1.0f,0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f} } }),
			 indices({ 0,1,2,2,1,3 }) {}
};
