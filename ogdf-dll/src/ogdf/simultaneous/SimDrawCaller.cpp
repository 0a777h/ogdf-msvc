/*
 * $Revision: 3521 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2013-05-31 14:52:33 +0200 (Fri, 31 May 2013) $
 ***************************************************************/

/** \file
 * \brief Offers variety of possible algorithm calls for simultaneous
 * drawing.
 *
 * \author Michael Schulz
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

#include<ogdf/simultaneous/SimDrawCaller.h>
#include<ogdf/layered/SugiyamaLayout.h>
#include<ogdf/planarity/PlanarizationLayout.h>
#include<ogdf/planarity/SubgraphPlanarizer.h>
#include<ogdf/planarity/VariableEmbeddingInserter.h>


namespace ogdf {

//*************************************************************
// refreshes m_esg
//
void SimDrawCaller::updateESG()
{
	edge e;
	forall_edges(e, *m_G)
		(*m_esg)[e] = m_GA->subGraphBits(e);

} // end updateESG


//*************************************************************
// Constructor
//
SimDrawCaller::SimDrawCaller(SimDraw &SD) : SimDrawManipulatorModule(SD)
{
	m_esg = OGDF_NEW EdgeArray<__uint32>(*m_G);
	updateESG();

} // end constructor


//*************************************************************
// call for SugiyamaLayout
void SimDrawCaller::callSugiyamaLayout()
{
	m_SD->addAttribute(GraphAttributes::nodeGraphics);
	m_SD->addAttribute(GraphAttributes::edgeGraphics);

	// nodes get default size
	node v;
	forall_nodes(v, *m_G)
		m_GA->height(v) = m_GA->width(v) = 5.0;

	// actual call of SugiyamaLayout
	updateESG();
	SugiyamaLayout SL;
	SL.setSubgraphs(m_esg); // needed to call SimDraw mode
	SL.call(*m_GA);

} // end callSugiyamaLayout


//*************************************************************
// call for PlanarizationLayoutUML
void SimDrawCaller::callPlanarizationLayout()
{
	m_SD->addAttribute(GraphAttributes::nodeGraphics);
	m_SD->addAttribute(GraphAttributes::edgeGraphics);

	// nodes get default size
	node v;
	forall_nodes(v, *m_G)
		m_GA->height(v) = m_GA->width(v) = 5.0;


	// actual call on PlanarizationLayout
	PlanarizationLayout PL;
	PL.callSimDraw(*m_GA);

} // end callPlanarizationLayout


//*************************************************************
// call for SubgraphPlanarizer
// returns crossing number
int SimDrawCaller::callSubgraphPlanarizer(int cc, int numberOfPermutations)
{
	// transfer edge costs if existent
	EdgeArray<int> ec(*m_G, 1);
	if(m_GA->attributes() & GraphAttributes::edgeIntWeight)
	{
		edge e;
		forall_edges(e,*m_G)
			ec[e] = m_GA->intWeight(e);
	}

	// initialize
	updateESG();
	int crossNum = 0;
	PlanRep PR(*m_G);

	// actual call for connected component cc
	SubgraphPlanarizer SP;
	VariableEmbeddingInserter* vei = new VariableEmbeddingInserter;
	vei->removeReinsert(rrIncremental);
	SP.setInserter(vei);
	SP.permutations(numberOfPermutations);
	SP.call(PR, cc, crossNum, &ec, 0, m_esg);

	// insert all dummy nodes into original graph *m_G
	NodeArray<node> newOrigNode(PR);
	node vPR;
	forall_nodes(vPR, PR)
	{
		if(PR.isDummy(vPR))
		{
			node vOrig = m_G->newNode();
			newOrigNode[vPR] = vOrig;
			m_SD->isDummy(vOrig) = true;
		}
		else
			newOrigNode[vPR] = PR.original(vPR);
		//original nodes are saved
	}

	// insert all edges incident to dummy nodes into *m_G
	EdgeArray<bool> toBeDeleted(*m_G, false);
	EdgeArray<bool> visited(PR, false);
	forall_nodes(vPR, PR)
	{
		if(PR.isDummy(vPR))
		{
			node vNewOrig = newOrigNode[vPR]; //lebt in *m_G
			edge e;
			forall_adj_edges(e, vPR) //lebt in PR
			{
				if(!visited[e])
				{
					node w = e->opposite(vPR); //lebt in PR
					node wNewOrig = newOrigNode[w]; //lebt in *m_G
					edge eNewOrig = m_G->newEdge(vNewOrig,wNewOrig);
					m_GA->subGraphBits(eNewOrig) = m_GA->subGraphBits(PR.original(e));
					toBeDeleted[PR.original(e)] = true;
					visited[e] = true;
				}
			}
		}
	}

	// delete all old edges in *m_G that are replaced by dummy node edges
	List<edge> LE;
	m_G->allEdges(LE);
	forall_listiterators(edge, it, LE)
	{
		if(toBeDeleted[ (*it) ])
			m_G->delEdge( (*it) );
	}

	return crossNum;

} // end callSubgraphPlanarizer

} // end namespace ogdf
