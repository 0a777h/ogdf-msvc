/*
 * $Revision: 3368 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2013-04-04 20:07:31 +0200 (Thu, 04 Apr 2013) $
 ***************************************************************/

/** \file
 * \brief implements class VariableEmbeddingInserterDyn
 *
 * \author Carsten Gutwenger
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


#include <ogdf/planarity/VariableEmbeddingInserterDyn.h>
#include <ogdf/internal/planarity/VarEdgeInserterDynCore.h>


namespace ogdf {

	//---------------------------------------------------------
	// constructor
	// sets default values for options
	//
	VariableEmbeddingInserterDyn::VariableEmbeddingInserterDyn()
	{
		m_rrOption = rrNone;
		m_percentMostCrossed = 25;
	}


	// copy constructor
	VariableEmbeddingInserterDyn::VariableEmbeddingInserterDyn(const VariableEmbeddingInserterDyn &inserter)
		: EdgeInsertionModule(inserter)
	{
		m_rrOption = inserter.m_rrOption;
		m_percentMostCrossed = inserter.m_percentMostCrossed;
	}


	// clone method
	EdgeInsertionModule *VariableEmbeddingInserterDyn::clone() const
	{
		return new VariableEmbeddingInserterDyn(*this);
	}


	// assignment operator
	VariableEmbeddingInserterDyn &VariableEmbeddingInserterDyn::operator=(const VariableEmbeddingInserterDyn &inserter)
	{
		m_rrOption = inserter.m_rrOption;
		m_percentMostCrossed = inserter.m_percentMostCrossed;
		return *this;
	}


	// actual call method
	Module::ReturnType VariableEmbeddingInserterDyn::doCall(
		PlanRepLight &pr,
		const Array<edge> &origEdges,
		const EdgeArray<int> *pCostOrig,
		const EdgeArray<bool> *pForbiddenOrig,
		const EdgeArray<__uint32> *pEdgeSubgraphs)
	{
		VarEdgeInserterDynCore core(pr, pCostOrig, pForbiddenOrig, pEdgeSubgraphs);
		core.timeLimit(timeLimit());

		ReturnType retVal = core.call(origEdges, m_rrOption, m_percentMostCrossed);
		m_runsPostprocessing = core.runsPostprocessing();
		return retVal;
	}


}
