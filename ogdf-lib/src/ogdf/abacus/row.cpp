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

#include <ogdf/abacus/row.h>

namespace abacus {


ostream &operator<<(ostream& out, const Row &rhs)
{
	double eps = rhs.glob_->machineEps();
	const int rhsNnz = rhs.nnz();

	for (int i = 0; i < rhsNnz; i++) {
		int    s = rhs.support(i);
		double c = rhs.coeff(i);

		char sign;
		if (c < 0.0) {
			sign = '-';
			c = -c;
		}
		else sign = '+';

		if (i > 0 || sign == '-')  // do not print first \a '+' of row
			out << sign << ' ';
		if (c < 1.0 - eps || 1.0 + eps < c)  // do not print coefficient 1
			out << c << ' ';
		out << 'x' << s << ' ';

		if (i && !(i % 10)) out << endl;
	}

	return out << rhs.sense_ << ' ' << rhs.rhs();
}


void Row::copy(const Row &row)
{
	sense_ = row.sense_;
	rhs_   = row.rhs_;

	SparVec::copy(row);
}
} //namespace abacus
