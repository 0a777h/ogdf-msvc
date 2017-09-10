/*
 * $Revision: 3096 $
 *
 * last checkin:
 *   $Author: chimani $
 *   $Date: 2012-11-30 15:40:41 +0100 (Fri, 30 Nov 2012) $
 ***************************************************************/

/** \file
 * \brief Logging functionality
 *
 * \author Markus Chimani
 *
 * \par License:
 * This file is part of the Open Graph Drawing Framework (OGDF).
 *
 * \par
 * Copyright (C)<br>
 * See README.txt in the root directory of the OGDF installation for details.
 *
 * \par
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 or 3 as published by the Free Software Foundation;
 * see the file LICENSE.txt included in the packaging of this file
 * for details.
 *
 * \par
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * \par
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * \see  http://www.gnu.org/copyleft/gpl.html
 ***************************************************************/


#include <ogdf/basic/Logger.h>

namespace ogdf {

// initializers
#ifdef OGDF_DEBUG
Logger::Level Logger::m_globalloglevel = Logger::LL_DEFAULT;
Logger::Level Logger::m_minimumloglevel = Logger::LL_MINOR;
Logger::Level Logger::m_globallibraryloglevel = Logger::LL_DEFAULT;
#else // RELEASE
Logger::Level Logger::m_globalloglevel = Logger::LL_ALARM; // forbid anything except alarms and forced writes -> logging is off
Logger::Level Logger::m_globallibraryloglevel = Logger::LL_ALARM;
Logger::Level Logger::m_minimumloglevel = Logger::LL_MEDIUM;
#endif

std::ostream* Logger::world = &std::cout;
std::ostream Logger::nirvana(NULL);

bool Logger::m_globalstatisticmode = false;

}

