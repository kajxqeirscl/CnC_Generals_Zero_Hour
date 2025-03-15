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
	// No experience for killing an ally, cheater.
	//if( killer->getRelationship( m_parent ) == ALLIES )
	//	return 0;

	VeterancyLevel m_vetLevel = m_parent->getVeterancyLevel();
	Int EXPToGive = m_parent->getTemplate()->getExperienceValue(m_vetLevel);
	Int NXPToGive = m_currentNXP / 2;
	if (EXPToGive >= NXPToGive) {
		return EXPToGive;
	}
	else {
		return NXPToGive;
	}
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
// Set Level to AT LEAST this... if we are already >= this level, do nothing.
void NXPTracker::setMinNXPLevel(Int newLevel)
{
	// This does not check for IsTrainable, because this function is for explicit setting,
	// so the setter is assumed to know what they are doing.  The game function
	// of addExperiencePoints cares about Trainability.
	if (m_currentLevel < newLevel)
	{
		Int oldLevel = m_currentLevel;
		m_currentLevel = newLevel;
		m_currentNXP = 100 * newLevel;//m_parent->getTemplate()->getExperienceRequired(m_currentLevel); //Minimum for this level
		if (m_parent)
			m_parent->onNXPLevelChanged(oldLevel, newLevel);
	}
}

//-------------------------------------------------------------------------------------------------
void NXPTracker::setNXPLevel(Int newLevel)
{
	// This does not check for IsTrainable, because this function is for explicit setting,
	// so the setter is assumed to know what they are doing.  The game function
	// of addExperiencePoints cares about Trainability, if flagged thus.
	if (m_currentLevel != newLevel)
	{
		Int oldLevel = m_currentLevel;
		m_currentLevel = newLevel;
		m_currentNXP = 100 * newLevel;//m_parent->getTemplate()->getExperienceRequired(m_currentLevel); //Minimum for this level
		if (m_parent)
			m_parent->onNXPLevelChanged(oldLevel, newLevel);
	}
}

//-------------------------------------------------------------------------------------------------
Bool NXPTracker::gainNXPForLevel(Int levelsToGain, Bool canScaleForBonus)
{
	Int newLevel = (Int)m_currentLevel + levelsToGain;
	// gain what levels we can, even if we can't use 'em all
	if (newLevel > m_currentLevel)
	{
		Int experienceNeeded = 100 * newLevel - m_currentNXP;
		addNXP(experienceNeeded, canScaleForBonus);
		return true;
	}
	return false;
}

//-------------------------------------------------------------------------------------------------
Bool NXPTracker::canGainNXPForLevel(Int levelsToGain) const
{
	Int newLevel = (Int)m_currentLevel + levelsToGain;
	// return true if we can gain levels, even if we can't gain ALL the levels requested
	return (newLevel > m_currentLevel);
}

//-------------------------------------------------------------------------------------------------
void NXPTracker::addNXP(Int experienceGain, Bool canScaleForBonus)
{
	if (m_NXPSink != INVALID_ID)
	{
		// I have been set up to give my experience to someone else
		Object* sinkPointer = TheGameLogic->findObjectByID(m_NXPSink);
		if (sinkPointer)
		{
			// Not a fatal failure if not valid, he died when I was in the air.
			sinkPointer->getNXPTracker()->addNXP(experienceGain * m_NXPScalar, canScaleForBonus);
			return;
		}
	}

	if (!isTrainable())
		return; //safety

	Int oldLevel = m_currentLevel;

	Int amountToGain = experienceGain;
	if (canScaleForBonus)
		amountToGain *= m_NXPScalar;


	m_currentNXP += amountToGain;

	Int levelIndex = 0;
	while (m_currentNXP >= 100 * levelIndex)
	{
		// If there is a higher level to qualify for, and I qualify for it, advance the index
		levelIndex++;
	}

	m_currentLevel = levelIndex-1;

	if (oldLevel != m_currentLevel)
	{
		// Edge trigger special level gain effects.
		m_parent->onNXPLevelChanged(oldLevel, m_currentLevel);
	}

}
//-------------------------------------------------------------------------------------------------
void NXPTracker::setNXPAndLevel(Int experienceIn)
{
	if (m_NXPSink != INVALID_ID)
	{
		// I have been set up to give my experience to someone else
		Object* sinkPointer = TheGameLogic->findObjectByID(m_NXPSink);
		if (sinkPointer)
		{
			// Not a fatal failure if not valid, he died when I was in the air.
			sinkPointer->getNXPTracker()->setNXPAndLevel(experienceIn);
			return;
		}
	}

	if (!isTrainable())
		return; //safety

	Int oldLevel = m_currentLevel;

	m_currentNXP = experienceIn;

	Int levelIndex = 0;
	while (m_currentNXP >= 100 * levelIndex)
	{
		// If there is a higher level to qualify for, and I qualify for it, advance the index
		levelIndex++;
	}

	m_currentLevel = levelIndex-1;

	if (oldLevel != m_currentLevel)
	{
		// Edge trigger special level gain effects.
		m_parent->onNXPLevelChanged(oldLevel, m_currentLevel); //<<== paradox! this may be a level lost!
	}

}

//-----------------------------------------------------------------------------
void NXPTracker::crc(Xfer* xfer)
{
	xfer->xferInt(&m_currentNXP);
	xfer->xferUser(&m_currentLevel, sizeof(Int));
}  // end crc

//-----------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version
	*/
	// ----------------------------------------------------------------------------
void NXPTracker::xfer(Xfer* xfer)
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion(&version, currentVersion);

	// no need to save the m_parent pointer, it is connected on allocation time
	// m_parent

	if (currentVersion >= 1) { //nxp save load
		// current level
		xfer->xferUser(&m_currentLevel, sizeof(Int));

		// current experience
		xfer->xferInt(&m_currentNXP);

		// experience sink
		xfer->xferObjectID(&m_NXPSink);

		// experience scalar
		xfer->xferReal(&m_NXPScalar);
	}

}  // end xfer

//-----------------------------------------------------------------------------
void NXPTracker::loadPostProcess(void)
{

}  // end loadPostProcess

