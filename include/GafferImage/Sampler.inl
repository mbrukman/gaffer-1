//////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2013, Image Engine Design Inc. All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are
//  met:
//
//      * Redistributions of source code must retain the above
//        copyright notice, this list of conditions and the following
//        disclaimer.
//
//      * Redistributions in binary form must reproduce the above
//        copyright notice, this list of conditions and the following
//        disclaimer in the documentation and/or other materials provided with
//        the distribution.
//
//      * Neither the name of Image Engine Design nor the names of
//        any other contributors to this software may be used to endorse or
//        promote products derived from this software without specific prior
//        written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
//  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
//  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
//  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
//  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
//  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
//  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
//  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//////////////////////////////////////////////////////////////////////////

#include "GafferImage/ImageAlgo.h"

namespace GafferImage
{

float Sampler::sample( float x, float y )
{
	// Perform an early-out for the box filter.
	if ( static_cast<GafferImage::TypeId>( m_filter->typeId() ) == GafferImage::BoxFilterTypeId )
	{
		return sample( IECore::fastFloatFloor( x ), IECore::fastFloatFloor( y ) );
	}

	// Otherwise do a filtered lookup.
	int tapX = m_filter->tap( x - m_cacheWindow.min.x );
	const int width = m_filter->width();
	float weightsX[width];

	for ( int i = 0; i < width; ++i )
	{
		weightsX[i] = m_filter->weight( x, tapX+i+m_cacheWindow.min.x );
	}

	int tapY = m_filter->tap( y - m_cacheWindow.min.y );
	const int height = m_filter->width();
	float weightsY[height];

	for ( int i = 0; i < height; ++i )
	{
		weightsY[i] = m_filter->weight( y, tapY+i+m_cacheWindow.min.y );
	}

	float weightedSum = 0.;
	float colour = 0.f;
	int absY = tapY + m_cacheWindow.min.y;
	for ( int y = 0; y < height; ++y, ++absY )
	{
		int absX = tapX + m_cacheWindow.min.x;
		for ( int x = 0; x < width; ++x, ++absX )
		{
			float c = 0.;
			float w = weightsX[x] * weightsY[y];
			c = sample( absX, absY );
			weightedSum += w;
			colour += c * w;
		}
	}

	return weightedSum == 0 ? 0 : colour / weightedSum;
}

float Sampler::sample( int x, int y )
{
	Imath::V2i p( x, y );

#ifndef NDEBUG

	// It is the caller's responsibility to ensure that sampling
	// is only performed within the sample window.
	assert( contains( m_sampleWindow, p ) );

#endif

	// Deal with lookups outside of the data window.
	if( m_boundingMode == Black && !contains( m_dataWindow, p ) )
	{
		return 0.0f;
	}
	else if( m_boundingMode == Clamp )
	{
		p = clamp( p, m_dataWindow );
	}

	const float *tileData;
	Imath::V2i tileOrigin;
	Imath::V2i tileIndex;
	cachedData( p, tileData, tileOrigin, tileIndex );
	return *(tileData + tileIndex.y * ImagePlug::tileSize() + tileIndex.x);
}

void Sampler::cachedData( Imath::V2i p, const float *& tileData, Imath::V2i &tileOrigin, Imath::V2i &tileIndex )
{
	// Get the smart pointer to the tile we want.
	p -= m_cacheWindow.min;
	Imath::V2i cacheIndex( p / Imath::V2i( ImagePlug::tileSize() ) );
	tileIndex = Imath::V2i( p - cacheIndex * Imath::V2i( ImagePlug::tileSize() ) );
	IECore::ConstFloatVectorDataPtr &cacheTilePtr = m_dataCache[ cacheIndex.x + cacheIndex.y * m_cacheWidth ];

	// Get the origin of the tile we want.
	tileOrigin = Imath::V2i( (( m_cacheWindow.min / ImagePlug::tileSize()) + cacheIndex ) * ImagePlug::tileSize() );
	if ( cacheTilePtr == NULL ) cacheTilePtr = m_plug->channelData( m_channelName, tileOrigin );

	tileData = &cacheTilePtr->readable()[0];
}

}; // namespace GafferImage


