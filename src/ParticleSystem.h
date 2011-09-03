#pragma once

#include "Particle.h"

#define MAX_PARTICLES		1000

using namespace MSA;

class ParticleSystem {
public:	
	
    float posArray[MAX_PARTICLES * 2 * 2];
    float colArray[MAX_PARTICLES * 3 * 2];
	Vec2f				windowSize;
	Vec2f				invWindowSize;
	const FluidSolver	*solver;
	
    int curIndex;
	
    Particle particles[MAX_PARTICLES];
	
	ParticleSystem();
	void setFluidSolver( const FluidSolver *aSolver ) { solver = aSolver; }
	
    void updateAndDraw( bool drawingFluid );
	void addParticles( const Vec2f &pos, int count );
	void addParticle( const Vec2f &pos );
	void setWindowSize( Vec2f winSize );
};
