/*
**	Command & Conquer Generals(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

////////////////////////////////////////////////////////////////////////////////
//																																						//
//  (c) 2001-2003 Electronic Arts Inc.																				//
//																																						//
////////////////////////////////////////////////////////////////////////////////

// FILE: ExperienceTracker.cpp //////////////////////////////////////////////////////////////////////
// Author: Graham Smallwood, February 2002
// Desc:   Keeps track of experience points so Veterance levels can be gained
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine

#include "Common/Xfer.h"
#include "Common/ThingTemplate.h"
#include "GameLogic/NXPTracker.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Object.h"


#ifdef _INTERNAL
// for occasional debugging...
//#pragma optimize("", off)
//#pragma MESSAGE("************************************** WARNING, optimization disabled for debugging purposes")
#endif

//-------------------------------------------------------------------------------------------------
NXPTracker::NXPTracker(Object* parent) :
    m_parent(parent),
    m_currentLevel(0),
    m_NXPSink(INVALID_ID),
    m_NXPScalar(1.0f),
    m_currentNXP(0) // Added By Sadullah Nader
{
}

//-------------------------------------------------------------------------------------------------
NXPTracker::~NXPTracker()
{
}

//-------------------------------------------------------------------------------------------------
Int NXPTracker::getNXPValue(const Object* killer) const
{
    VeterancyLevel m_vetLevel = m_parent->getVeterancyLevel();
    Int EXPToGive = m_parent->getTemplate()->getExperienceValue(m_vetLevel);
    Int NXPToGive = m_currentNXP / 2;
    return (EXPToGive >= NXPToGive) ? EXPToGive : NXPToGive;
}

//-------------------------------------------------------------------------------------------------
Bool NXPTracker::isTrainable() const
{
    return m_parent->getTemplate()->isTrainable();
}

//-------------------------------------------------------------------------------------------------
Bool NXPTracker::isAcceptingNXP() const
{
    return isTrainable() || (m_NXPSink != INVALID_ID);
}

//-------------------------------------------------------------------------------------------------
void NXPTracker::setNXPSink(ObjectID sink)
{
    m_NXPSink = sink;
}

//-------------------------------------------------------------------------------------------------
ObjectID NXPTracker::getNXPSink() const
{
    return m_NXPSink;
}

//-------------------------------------------------------------------------------------------------
void NXPTracker::setMinNXPLevel(Int newLevel)
{
    if (m_currentLevel < newLevel)
    {
        Int oldLevel = m_currentLevel;
        m_currentLevel = newLevel;
        m_currentNXP = static_cast<Int>(100 * pow(1.3, newLevel - 1));
        if (m_parent)
            m_parent->onNXPLevelChanged(oldLevel, newLevel);
    }
}

//-------------------------------------------------------------------------------------------------
void NXPTracker::setNXPLevel(Int newLevel)
{
    if (m_currentLevel != newLevel)
    {
        Int oldLevel = m_currentLevel;
        m_currentLevel = newLevel;
        m_currentNXP = static_cast<Int>(100 * pow(1.3, newLevel - 1));
        if (m_parent)
            m_parent->onNXPLevelChanged(oldLevel, newLevel);
    }
}

//-------------------------------------------------------------------------------------------------
Bool NXPTracker::gainNXPForLevel(Int levelsToGain, Bool canScaleForBonus)
{
    Int newLevel = m_currentLevel + levelsToGain;
    if (newLevel > m_currentLevel)
    {
        Int experienceNeeded = static_cast<Int>(100 * pow(1.3, newLevel - 1)) - m_currentNXP;
        addNXP(experienceNeeded, canScaleForBonus);
        return true;
    }
    return false;
}

//-------------------------------------------------------------------------------------------------
Bool NXPTracker::canGainNXPForLevel(Int levelsToGain) const
{
    Int newLevel = m_currentLevel + levelsToGain;
    return (newLevel > m_currentLevel);
}

//-------------------------------------------------------------------------------------------------
void NXPTracker::addNXP(Int experienceGain, Bool canScaleForBonus)
{
    if (m_NXPSink != INVALID_ID)
    {
        Object* sinkPointer = TheGameLogic->findObjectByID(m_NXPSink);
        if (sinkPointer)
        {
            sinkPointer->getNXPTracker()->addNXP(experienceGain * m_NXPScalar, canScaleForBonus);
            return;
        }
    }
    if (!isTrainable()) return;

    Int oldLevel = m_currentLevel;
    Int amountToGain = canScaleForBonus ? experienceGain * m_NXPScalar : experienceGain;
    m_currentNXP += amountToGain;

    Int levelIndex = 0;
    while (m_currentNXP >= static_cast<Int>(100 * pow(1.3, levelIndex)))
    {
        levelIndex++;
    }
    m_currentLevel = levelIndex;

    if (oldLevel != m_currentLevel)
    {
        m_parent->onNXPLevelChanged(oldLevel, m_currentLevel);
    }
}

//-------------------------------------------------------------------------------------------------
void NXPTracker::setNXPAndLevel(Int experienceIn)
{
    if (m_NXPSink != INVALID_ID)
    {
        Object* sinkPointer = TheGameLogic->findObjectByID(m_NXPSink);
        if (sinkPointer)
        {
            sinkPointer->getNXPTracker()->setNXPAndLevel(experienceIn);
            return;
        }
    }
    if (!isTrainable()) return;

    Int oldLevel = m_currentLevel;
    m_currentNXP = experienceIn;

    Int levelIndex = 0;
    while (m_currentNXP >= static_cast<Int>(100 * pow(1.3, levelIndex)))
    {
        levelIndex++;
    }
    m_currentLevel = levelIndex;

    if (oldLevel != m_currentLevel)
    {
        m_parent->onNXPLevelChanged(oldLevel, m_currentLevel);
    }
}

//----------------------------------------------------------------------------- 
void NXPTracker::crc(Xfer* xfer)
{
    xfer->xferInt(&m_currentNXP);
    xfer->xferUser(&m_currentLevel, sizeof(Int));
}

//----------------------------------------------------------------------------- 
void NXPTracker::xfer(Xfer* xfer)
{
    XferVersion currentVersion = 1;
    XferVersion version = currentVersion;
    xfer->xferVersion(&version, currentVersion);

    if (currentVersion >= 1) {
        xfer->xferUser(&m_currentLevel, sizeof(Int));
        xfer->xferInt(&m_currentNXP);
        xfer->xferObjectID(&m_NXPSink);
        xfer->xferReal(&m_NXPScalar);
    }
}

//----------------------------------------------------------------------------- 
void NXPTracker::loadPostProcess(void)
{
}

