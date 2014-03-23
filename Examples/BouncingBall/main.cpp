//  Copyright (C) 2014 Manuel Riecke <spell1337@gmail.com>
//  Licensed under the terms of the WTFPL.
//
//  TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION
//  0. You just DO WHAT THE FUCK YOU WANT TO.

#include <Springbok/Platform/InputMonitor.h>
#include <Springbok/Platform/GameSurface.h>
#include <Springbok/Platform/PreciseClock.h>
#include <Springbok/Platform/FileInfo.h>

#include <Springbok/Graphics/RenderContext2D.h>
#include <Springbok/Graphics/Image.h>
#include <Springbok/Graphics/BatchRenderer.h>

#include <Springbok/Resources/ResourceManager.h>

#include "data.h"

int main()
{
	GameSurface surface("BouncingBall - Springbok Example", GameSurface::Windowed);
	InputMonitor       input(&surface);
	RenderContext2D renderer(&surface);
	
	ResourceManager::GetInstance()->findPrimaryResourcePath({FileInfo(__FILE__).up()+"/Assets", "./Assets"});
	
	InitData();
	
	PreciseClock timer;
	float currentTime = timer.elapsed();
	timer.start();
	
	Image ball("Ball.png");
	Image bg("BG.png");
	Image shadow("Shadow.png");
	BatchRenderer2D batcher;
	
	while(!surface.closeRequested())
	{
		surface.switchBuffers();
		renderer.clear();
		
		float deltaTime = timer.elapsed();
		timer.start();
		currentTime += deltaTime;
		
		batcher.startBatching(renderer);
		{
			batcher.addToBatch(bg, {0});
			
			Transform2D transformation;
			transformation.Offset = Vec2F(-400 + currentTime * 350, +200);
			transformation.Scale = BallScale[currentTime];

			batcher.addToBatch(shadow, transformation);
			transformation.Offset.Y = 180 - BallHeight[currentTime];
			batcher.addToBatch(ball, transformation);
		}
		batcher.flushBatches();
	}
};