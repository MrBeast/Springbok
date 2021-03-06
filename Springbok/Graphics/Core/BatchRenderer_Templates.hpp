//  Copyright (C) 2014 Manuel Riecke <spell1337@gmail.com>
//  Licensed under the terms of the WTFPL.
//
//  TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION
//  0. You just DO WHAT THE FUCK YOU WANT TO.

#pragma once

#include "BatchRenderer.h"
#include "GLES2.h"
#include "RenderContext2D.h"

template<class E, class V>
BatchRenderer<E,V>::BatchRenderer(int bytes) // Bytes is 2 MB by default
{
	mMaxVertices = bytes / sizeof(V);
	if(mMaxVertices > 32767) // With short indices we can't have more than 65k Vertices!
		mMaxVertices = 32767;  // BUG: Some GPU drivers only support 32k vertices appeareantly.
	
	mMaxElements = mMaxVertices / 16; // In the case of 2MB Vertices = Up to 4096 batches,    48 kB
	mMaxIndices  = mMaxVertices *  2; // In the case of 2MB Vertices = Up to 131072 indices, 256 kB
	
	mExtraVertices = 128;
	
	mVertexData  = new V       [mMaxVertices + mExtraVertices]; // 2MB = 65536 of the default vertices (32 bytes per Vertex)
	mElementData = new E       [mMaxElements + 2]; 
	mIndexData   = new GLushort[mMaxIndices  + mExtraVertices*2]; 
	
	glGenVertexArrays(1, &mVertexArray);
	glBindVertexArray(mVertexArray);

	glGenBuffers(1, &mVertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, mMaxVertices * sizeof(V), NULL, GL_DYNAMIC_DRAW); 
	PrintGLError();
	
	glGenBuffers(1, &mIndexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mMaxIndices * sizeof(GLushort), NULL, GL_DYNAMIC_DRAW); 
	PrintGLError();
};

template<class E, class V>
template<typename T>
void BatchRenderer<E,V>::draw(const T& object, Transform2D transformation, const V& vertex, const E& element)
{	
	RenderDataPointer<V, E> oldParams = mParams;
	
	mParams.DefaultElement = element;
	mParams.DefaultVertex  = vertex;
	mParams.updateDefaults();
	
	// Get the data before it is changed by prepareVertices.
	V* oldVertices = mVertexData + mParams.AddedVertices;
	
	object.prepareVertices(mParams);
	
	if(mParams.AddedVertices > mMaxVertices)
	{
		//DebugLog("BatchRenderer: Wrote more vertices than we could handle. Flushing all batches and restarting.");
		// Rewind, flush, and press play again
		mParams = oldParams;
		flushBatches();
		draw<T>(object, transformation, vertex, element);
	}
	else
		transformation.transform(oldVertices, mParams.Vertices, mCurrentContext->cameraCenter());
};

template<class E, class V>
template<typename T>
void BatchRenderer<E,V>::drawRaw(const T& object, const V& vertex, const E& element)
{	
	RenderDataPointer<V, E> oldParams = mParams;
	
	mParams.DefaultElement = element;
	mParams.DefaultVertex  = vertex;
	mParams.updateDefaults();
	
	// Get the data before it is changed by prepareVertices.
	V* oldVertices = mVertexData + mParams.AddedVertices;
	
	object.prepareVertices(mParams);
	
	if(mParams.AddedVertices > mMaxVertices)
	{
		//DebugLog("BatchRenderer: Wrote more vertices than we could handle. Flushing all batches and restarting.");
		// Rewind, flush, and press play again
		mParams = oldParams;
		flushBatches();
		drawRaw<T>(object, vertex, element);
	}
};

template<class E, class V>
void BatchRenderer<E,V>::drawRect(Rect vertices, Rect texCoords, unsigned int texture, const V& vertex, E element)
{
	RenderDataPointer<V, E> oldParams = mParams;
	
	mParams.DefaultElement = element;
	mParams.DefaultVertex  = vertex;
	mParams.updateDefaults();
	
	V* oldVertices = mVertexData + mParams.AddedVertices;
	
	for(int i = 0; i < 4; ++i)
	{
		mParams.Vertices->Position  = vertices.Points[i];
		mParams.Vertices->TexCoords = texCoords.Points[i];
		mParams.appendVertex();
	}
	mParams.appendIndex(0);
	mParams.appendIndex(1);
	mParams.appendIndex(2);
	mParams.appendIndex(3);
	
	element.Texture = texture;
	mParams.appendElement(element);
	
	if(mParams.AddedVertices > mMaxVertices)
	{
		mParams = oldParams;
		flushBatches();
		drawRect(vertices, texCoords, texture, vertex, element);
	}
};

template<class E, class V>
void BatchRenderer<E,V>::flushBatches()
{
	// Skip if nothing would be drawn
	if(mParams.AddedVertices <= 2)
		return;
	
	if(!mGLStateIsSet)
	{
		glBindVertexArray(mVertexArray);
		glBindBuffer(GL_ARRAY_BUFFER,         mVertexBuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,  mIndexBuffer);
		PrintGLError();
		V::SetupOffsets();
		PrintGLError();
		E::SetupUniforms(mCurrentContext);
		PrintGLError();
		mGLStateIsSet = true;
	}
	
	glBufferSubData(GL_ARRAY_BUFFER,         0, Min(mParams.AddedVertices, mMaxVertices) * sizeof(V), mVertexData);
	PrintGLError();
	
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, Min(mParams.AddedIndices, mMaxIndices) * sizeof(GLushort), mIndexData);
	
	PrintGLError();
	E* last = (mElementData + Min(mParams.AddedElements, mMaxElements));
	for(E* it = mElementData; it < last; ++it)
	{
		it->bindUniforms();
		PrintGLError();
		GLushort numIndices = (it->IndexEnd - it->IndexStart) - 1;
		glDrawElements(GL_TRIANGLE_STRIP, numIndices, GL_UNSIGNED_SHORT, (const void*)((it->IndexStart - mIndexData)*sizeof(GLushort)));
	}
	PrintGLError();
	
	mParams = RenderDataPointer<V, E>(mVertexData, mElementData, mIndexData);
};

template<class E, class V>
void BatchRenderer<E,V>::startBatching(const RenderContext2D& context)
{
	mCurrentContext = &context;
	mParams = RenderDataPointer<V, E>(mVertexData, mElementData, mIndexData);
	mParams.updateDefaults();
	mGLStateIsSet = false;
};