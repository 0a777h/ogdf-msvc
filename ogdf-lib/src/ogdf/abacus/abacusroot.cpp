/*
 * $Revision: 3386 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2013-04-10 14:00:02 +0200 (Wed, 10 Apr 2013) $
 ***************************************************************/

/*!\file
 * \author Matthias Elf
 *
 * \par License:
 * This file is part of ABACUS - A Branch And CUt System
 * Copyright (C) 1995 - 2003
 * University of Cologne, Germany
 *
 * \par
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * \par
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * \par
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * \see http://www.gnu.org/copyleft/gpl.html
 */

#include <ogdf/abacus/abacusroot.h>

namespace abacus {


const char *AbacusRoot::onOff(bool value)
{
	return (value) ? "on" : "off";
}


bool AbacusRoot::ascii2bool(const string &str)
{
	if (str == "true") return true;
	if (str == "false") return false;
	else {
		Logger::ifout() << "AbacusRoot::ascii2bool(): string neither true nor false\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::afcString);
	}
}


bool AbacusRoot::endsWith(const string &str, const string &end)
{
	string::size_type l1 = str.size();
	string::size_type l2 = end.size();

	if (l1 < l2)
		return false;

	return std::equal(str.begin()+(l1-l2), str.end(), end.begin());
}
} //namespace abacus
