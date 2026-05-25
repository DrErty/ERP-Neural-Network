#pragma once

#include "ERP.h"

class Game
{
public:
    virtual ~Game() = default;

    virtual uint32_t AddPlayer(bool display) = 0;

    virtual void Action(uint32_t playerIndex, uint32_t outputIndex) = 0;
    virtual float GetInput(uint32_t playerIndex, uint32_t inputIndex) const = 0;

    virtual bool PlayerAlive(uint32_t playerIndex) const = 0;
    virtual double PlayerFitness(uint32_t playerIndex) const = 0;
    virtual uint32_t PlayerCount() const = 0;
    virtual uint32_t AliveCount() const = 0;

    virtual void Step(float dt) = 0;
    virtual void Render() = 0;

    virtual bool IsDone() const = 0;
    virtual double GetSimTime() const = 0;

    virtual void Reset() = 0;
    virtual void KillPlayer(uint32_t playerIndex) = 0;
};