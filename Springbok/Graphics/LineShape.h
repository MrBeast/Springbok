//  Copyright (C) 2014 Manuel Riecke <spell1337@gmail.com>
//  Licensed under the terms of the WTFPL.
//
//  TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION
//  0. You just DO WHAT THE FUCK YOU WANT TO.

#pragma once

#include <vector>
#include <Springbok/Graphics/Core/VertexStruct.h>
#include <Springbok/Graphics/Image.h>
#include <Springbok/Utils/Color.h>

struct LineStyle
{
	LineStyle(Image img, int start, int end);
	LineStyle(){};
	Image TexImage;
	int   CenterStartPx = -1;
	int   CenterEndPx   = -1;
	float StartLength    = -1;
	float RepitionLength = -1;
	float EndLength      = -1;
};

struct LineShape
{
public:
	struct Point
	{
		Vec2F Position = 0.f;
		float    Width = 1.f;
		float TexCoord = 0.f;
		Vec4F Color = 1.f;
		Point blend(const Point& other, float factor);
	};
public:
	std::vector<Point> Points;
	Image              TexImage;
public:
	void  insert(Vec2F position, float width=3, Vec4F color=1);
	void  divideLinear(float pixels, int maxSegments=500);
	void  divideSmooth(float pixels, int maxSegments=500);
	void  divideCubicBezier(float pixels);
	void  applyTexture(const Image& img, float repetitions = 1.f);
	void  applyStyle(const LineStyle& style);
	float calcLength();
public:
	static LineShape Arrow(Vec2F vec, float width = 3.f, Vec4F endClr = Colors::White, Vec4F startClr = Colors::White);
	static LineShape Arrow(Vec2F vec, const Image& arrowImg, Vec4F clr = Colors::White, float width = -1.f);
	static LineShape Arrow(Vec2F vec, LineStyle style, Vec4F clr = Colors::White, float width = -1.f);
	static LineShape TurnIndicator(Angle turns, float radius, LineStyle style, Vec4F clr = Colors::White, float width = -1.f);
public:
	template<class V = BasicVertex, class E = BasicElement>
	void prepareVertices(RenderDataPointer< V, E >& data) const;
};

#include "LineShape_Templates.hpp"
