/*
* $Revision: 3159 $
*
* last checkin:
*   $Author: gutwenger $
*   $Date: 2012-12-13 09:01:58 +0100 (Thu, 13 Dec 2012) $
***************************************************************/

/** \file
 * \brief implementation of class PlanarizationLayoutUML.
 *
 * applies planarization approach for drawing UML diagrams
 * by calling a planar layouter for every planarized connected
 * component
 * Static and incremental calls available
 * Replaces cliques (if m_processCliques is set) to speed up
 * the computation, this does only work in non-UML mode
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


#include <ogdf/uml/PlanarizationLayoutUML.h>
#include <ogdf/uml/OrthoLayoutUML.h>
#include <ogdf/uml/SubgraphPlanarizerUML.h>
#include <ogdf/planarity/SimpleEmbedder.h>
#include <ogdf/packing/TileToRowsCCPacker.h>
#include <ogdf/basic/TopologyModule.h>
#include <ogdf/basic/precondition.h>
#include <ogdf/basic/simple_graph_alg.h>


namespace ogdf {

//-----------------------------------------------------------------------------
// Constructor:
// set default values for parameters and set planar layouter, planarization modules
//-----------------------------------------------------------------------------

PlanarizationLayoutUML::PlanarizationLayoutUML()
{
	//modules
	m_crossMin      .set(new SubgraphPlanarizerUML);
	m_planarLayouter.set(new OrthoLayoutUML);
	m_packer        .set(new TileToRowsCCPacker);
	m_embedder      .set(new SimpleEmbedder);

	//parameters
	m_pageRatio = 1.0;
	m_fakeTree  = true; //change edge type from generalization to association
}


//-----------------------------------------------------------------------------
// call function: compute a layout for graph umlGraph without
// special UML or interactive features, clique processing etc.
//-----------------------------------------------------------------------------

void PlanarizationLayoutUML::doSimpleCall(GraphAttributes &GA)
{
	m_nCrossings = 0;

	if(GA.constGraph().empty())
		return;

	PlanRepUML pr =  PlanRepUML(GA);
	const int numCC = pr.numberOfCCs();

	// (width,height) of the layout of each connected component
	Array<DPoint> boundingBox(numCC);

	//------------------------------------------
	//now planarize CCs and apply drawing module
	for(int i = 0; i < numCC; ++i)
	{
		//---------------------------------------
		// 1. crossing minimization
		//---------------------------------------
		int cr;
		m_crossMin.get().call(pr, i, cr);
		m_nCrossings += cr;


		//---------------------------------------
		// 2. embed resulting planar graph
		//---------------------------------------
		adjEntry adjExternal = 0;
		m_embedder.get().call(pr, adjExternal);


		//---------------------------------------------------------
		// 3. compute layout of planarized representation
		//---------------------------------------------------------

		Layout drawing(pr);

		//call the Layouter for the CC's UMLGraph
		m_planarLayouter.get().call(pr,adjExternal,drawing);

		// copy layout into umlGraph
		// Later, we move nodes and edges in each connected component, such
		// that no two overlap.

		for(int j = pr.startNode(); j < pr.stopNode(); ++j) {
			node vG = pr.v(j);

			GA.x(vG) = drawing.x(pr.copy(vG));
			GA.y(vG) = drawing.y(pr.copy(vG));

			adjEntry adj;
			forall_adj(adj,vG) {
				if ((adj->index() & 1) == 0)
					continue;
				edge eG = adj->theEdge();
				drawing.computePolylineClear(pr, eG, GA.bends(eG));
			}
		}

		// the width/height of the layout has been computed by the planar
		// layout algorithm; required as input to packing algorithm
		boundingBox[i] = m_planarLayouter.get().getBoundingBox();
	}

	//----------------------------------------
	// 4. arrange layouts of connected components
	//----------------------------------------

	arrangeCCs(pr, GA, boundingBox);
}



//-----------------------------------------------------------------------------
// call function: compute an UML layout for graph umlGraph
//-----------------------------------------------------------------------------
void PlanarizationLayoutUML::call(UMLGraph &umlGraph)
{
	m_nCrossings = 0;

	if(umlGraph.constGraph().empty())
		return;

	// check necessary preconditions
	preProcess(umlGraph);

	//---------------------------------------------------
	// preprocessing: insert a merger for generalizations
	umlGraph.insertGenMergers();

	PlanRepUML pr(umlGraph);
	const int numCC = pr.numberOfCCs();

	// (width,height) of the layout of each connected component
	Array<DPoint> boundingBox(numCC);


	// alignment section (should not be here, because planarlayout should
	// not know about the meaning of layouter options and should not cope
	// with them), move later
	// we have to distinguish between cc's with and without generalizations
	// if the alignment option is set
	int l_layoutOptions = m_planarLayouter.get().getOptions();
	bool l_align = ((l_layoutOptions & umlOpAlign) > 0);
	//end alignment section

	//------------------------------------------
	//now planarize CCs and apply drawing module
	for(int i = 0; i < numCC; ++i)
	{
		//---------------------------------------
		// 1. crossing minimization
		//---------------------------------------

		// alignment: check wether gens exist, special treatment is necessary
		bool l_gensExist = false; // set this for all CC's, start with first gen,
		//this setting can be mixed among CC's without problems

		EdgeArray<Graph::EdgeType> savedType(pr);
		EdgeArray<Graph::EdgeType> savedOrigType(pr.original()); //for deleted copies

		EdgeArray<int> costOrig(pr.original(), 1);
		//edgearray for reinserter call: which edge may never be crossed?
		EdgeArray<bool> noCrossingEdge(pr.original(), false);

		edge e;
		forall_edges(e,pr)
		{
			edge eOrig = pr.original(e);

			if (pr.typeOf(e) == Graph::generalization)
			{
				if (l_align) l_gensExist = true;
				OGDF_ASSERT(!eOrig || !(noCrossingEdge[eOrig]));

				// high cost to allow alignment without crossings
				if (l_align && (
					(eOrig && (pr.typeOf(e->target()) == Graph::generalizationMerger))
						|| pr.alignUpward(e->adjSource())
					))
				 costOrig[eOrig] = 10;

			}
		}

		int cr;
		m_crossMin.get().call(pr, i, cr, &costOrig);
		m_nCrossings += cr;


		//---------------------------------------
		// 2. embed resulting planar graph
		//---------------------------------------

		// We currently compute any embedding and choose the maximal face as external face

		// if we use FixedEmbeddingInserter, we have to re-use the computed
		// embedding, otherwise crossing nodes can turn into "touching points"
		// of edges (alternatively, we could compute a new embedding and
		// finally "remove" such unnecessary crossings).
		if(!pr.representsCombEmbedding())
			planarEmbed(pr);

		adjEntry adjExternal = 0;
		if(pr.numberOfEdges() > 0) {
			CombinatorialEmbedding E(pr);
			face fExternal = findBestExternalFace(pr,E);
			adjExternal = fExternal->firstAdj();
		}


		//---------------------------------------------------------
		// 3. compute layout of planarized representation
		//---------------------------------------------------------

		Layout drawing(pr);

		// distinguish between CC's with/without generalizations
		// this changes the input layout modules options!
		if (l_gensExist)
			m_planarLayouter.get().setOptions(l_layoutOptions);
		else
			m_planarLayouter.get().setOptions((l_layoutOptions & ~umlOpAlign));

		// call the Layouter for the CC's UMLGraph
		m_planarLayouter.get().call(pr, adjExternal, drawing);

		// copy layout into umlGraph
		// Later, we move nodes and edges in each connected component, such
		// that no two overlap.

		for(int j = pr.startNode(); j < pr.stopNode(); ++j) {
			node vG = pr.v(j);

			umlGraph.x(vG) = drawing.x(pr.copy(vG));
			umlGraph.y(vG) = drawing.y(pr.copy(vG));

			adjEntry adj;
			forall_adj(adj,vG) {
				if ((adj->index() & 1) == 0) continue;
				edge eG = adj->theEdge();

				drawing.computePolylineClear(pr,eG,umlGraph.bends(eG));
			}
		}

		// the width/height of the layout has been computed by the planar
		// layout algorithm; required as input to packing algorithm
		boundingBox[i] = m_planarLayouter.get().getBoundingBox();
	}//for cc's

	//----------------------------------------
	// Arrange layouts of connected components
	//----------------------------------------

	arrangeCCs(pr, umlGraph, boundingBox);

	umlGraph.undoGenMergers();
	umlGraph.removeUnnecessaryBendsHV();

	//new position after adding of cliqueprocess, check if correct
	postProcess(umlGraph);
}





//-----------------------------------------------------------------------------
// additional helper functions
//-----------------------------------------------------------------------------

void PlanarizationLayoutUML::assureDrawability(UMLGraph &UG)
{
	//preliminary
	//self loops are killed by the caller (e.g., the plugin interface)
	//should be done here later

	const Graph& G = UG.constGraph();

	//check for selfloops and handle them
	edge e;
	forall_edges(e, G) {
		if (e->isSelfLoop())
			OGDF_THROW_PARAM(PreconditionViolatedException, pvcSelfLoop);
	}

	// check for generalization - nontrees
	// if m_fakeTree is set, change type of "back" edges to association
	m_fakedGens.clear();//?
	if (!dfsGenTree(UG, m_fakedGens, m_fakeTree))
		OGDF_THROW_PARAM(PreconditionViolatedException, pvcTreeHierarchies);

	else {
		ListConstIterator<edge> itE = m_fakedGens.begin();
		while (itE.valid()) {
			UG.type(*itE) = Graph::association;
			itE++;
		}
	}
}//assureDrawability



void PlanarizationLayoutUML::preProcess(UMLGraph &UG)
{
	assureDrawability(UG);

	const SListPure<UMLGraph::AssociationClass*> &acList = UG.assClassList();
	SListConstIterator<UMLGraph::AssociationClass*> it = acList.begin();
	while (it.valid())
	{
		UG.modelAssociationClass((*it));
		it++;
	}
}//preprocess


void PlanarizationLayoutUML::postProcess(UMLGraph& UG)
{
	//reset the type of faked associations to generalization
	if (m_fakeTree)
	{
		ListIterator<edge> itE = m_fakedGens.begin();
		while (itE.valid())
		{
			UG.type(*itE) = Graph::generalization;
			itE++;
		}
	}

	UG.undoAssociationClasses();
}//postProcess


// find best suited external face according to certain criteria
face PlanarizationLayoutUML::findBestExternalFace(
	const PlanRep &PG,
	const CombinatorialEmbedding &E)
{
	FaceArray<int> weight(E);

	face f;
	forall_faces(f,E)
		weight[f] = f->size();

	node v;
	forall_nodes(v,PG)
	{
		if(PG.typeOf(v) != Graph::generalizationMerger)
			continue;

		adjEntry adj;
		forall_adj(adj,v) {
			if(adj->theEdge()->source() == v)
				break;
		}

		OGDF_ASSERT(adj->theEdge()->source() == v);

		node w = adj->theEdge()->target();
		bool isBase = true;

		adjEntry adj2;
		forall_adj(adj2, w) {
			edge e = adj2->theEdge();
			if(e->target() != w && PG.typeOf(e) == Graph::generalization) {
				isBase = false;
				break;
			}
		}

		if(isBase == false)
			continue;

		face f1 = E.leftFace(adj);
		face f2 = E.rightFace(adj);

		weight[f1] += v->indeg();
		if(f2 != f1)
			weight[f2] += v->indeg();
	}

	face fBest = E.firstFace();
	forall_faces(f,E)
		if(weight[f] > weight[fBest])
			fBest = f;

	return fBest;
}


void PlanarizationLayoutUML::arrangeCCs(PlanRep &PG, GraphAttributes &GA, Array<DPoint> &boundingBox)
{
	int numCC = PG.numberOfCCs();
	Array<DPoint> offset(numCC);
	m_packer.get().call(boundingBox,offset,m_pageRatio);

	// The arrangement is given by offset to the origin of the coordinate
	// system. We still have to shift each node and edge by the offset
	// of its connected component.

	for(int i = 0; i < numCC; ++i) {

		const double dx = offset[i].m_x;
		const double dy = offset[i].m_y;

		// iterate over all nodes in ith CC
		for(int j = PG.startNode(i); j < PG.stopNode(i); ++j)
		{
			node v = PG.v(j);

			GA.x(v) += dx;
			GA.y(v) += dy;

			adjEntry adj;
			forall_adj(adj,v) {
				if ((adj->index() & 1) == 0) continue;
				edge e = adj->theEdge();

				DPolyline &dpl = GA.bends(e);
				ListIterator<DPoint> it;
				for(it = dpl.begin(); it.valid(); ++it) {
					(*it).m_x += dx;
					(*it).m_y += dy;
				}
			}
		}
	}
}



//=========================================================
// removed stuff
//

#if 0

//---------------------------------------------------------
//compute a layout with the given embedding, take special
//care of  crossings (assumes that all crossings are non-ambiguous
//and can be derived from node/bend positions)
//---------------------------------------------------------
void PlanarizationLayoutUML::callFixEmbed(UMLGraph &umlGraph)
{
	m_nCrossings = 0;

	if(((const Graph &)umlGraph).empty())
		return;

	//check necessary preconditions
	preProcess(umlGraph);

	int l_layoutOptions = m_planarLayouter.get().getOptions();
	bool l_align = ((l_layoutOptions & umlOpAlign)>0);

	//--------------------------------------------------------
	//first, we sort all edges around each node corresponding
	//to the given layout in umlGraph
	//then we insert the mergers
	//try to rebuild this: insert mergers only in copy
	bool umlMerge = false; //Standard: false
	int i;

	//*********************************************************
	// first phase: Compute planarized representation from input
	//*********************************************************

	//now we derive a planar representation of the input
	//graph using its layout information
	PlanRepUML PG(umlGraph);

	const int numCC = PG.numberOfCCs();

	// (width,height) of the layout of each connected component
	Array<DPoint> boundingBox(numCC);


	//******************************************
	//now planarize CCs and apply drawing module
	for(i = 0; i < numCC; ++i)
	{
		// we treat connected component i
		// PG is set to a copy of this CC

		PG.initCC(i);

		int nOrigVerticesPG = PG.numberOfNodes();

		//alignment: check wether gens exist, special treatment is necessary
		bool l_gensExist = false; //set this for all CC's, start with first gen,
		//this setting can be mixed among CC's without problems

		//*********************************************************
		// we don't need to compute a planar subgraph, because we
		// use the given embedding
		//*********************************************************

		adjEntry adjExternal = 0;

		//here lies the main difference to the other call
		//we first planarize by using the given layout and then
		//set the embedding corresponding to the input
		bool embedded;
		TopologyModule TM;
		try {
			embedded = TM.setEmbeddingFromGraph(PG, umlGraph, adjExternal, umlMerge);
		}//try
		catch (...)
		{
			//TODO: check for graph changes that are not undone
			embedded = false;
		}


		//-------------------------------------------------
		//if not embedded correctly
		if (!embedded)
		{
			reembed(PG, i, l_align, l_gensExist);
		}//if !embedded

		//-------------------------------------------------

		CombinatorialEmbedding E(PG);

		if (!umlMerge)
			PG.setupIncremental(i, E);

		//
		// determine embedding of PG
		//

		// We currently compute any embedding and choose the maximal face
		// as external face

		// if we use FixedEmbeddingInserterOld, we have to re-use the computed
		// embedding, otherwise crossing nodes can turn into "touching points"
		// of edges (alternatively, we could compute a new embedding and
		// finally "remove" such unnecessary crossings).

		if((adjExternal == 0) && PG.numberOfEdges() > 0)
		{
			//face fExternal = E.maximalFace();
			face fExternal = findBestExternalFace(PG,E);
			adjExternal = fExternal->firstAdj();
			//while (PG.sinkConnect(adjExternal->theEdge()))
			//	adjExternal = adjExternal->faceCycleSucc();
		}

		m_nCrossings += PG.numberOfNodes() - nOrigVerticesPG;


		//*********************************************************
		// third phase: Compute layout of planarized representation
		//*********************************************************

		Layout drawing(PG);

		//distinguish between CC's with/without generalizations
		//this changes the input layout modules options!
		if (l_gensExist)
			m_planarLayouter.get().setOptions(l_layoutOptions);
		else m_planarLayouter.get().setOptions((l_layoutOptions & ~umlOpAlign));

		//***************************************
		//call the Layouter for the CC's UMLGraph
		m_planarLayouter.get().call(PG,adjExternal,drawing);

		// copy layout into umlGraph
		// Later, we move nodes and edges in each connected component, such
		// that no two overlap.

		//set position for original nodes and set bends for
		//all edges
		for(int j = PG.startNode(); j < PG.stopNode(); ++j)
		{
			node vG = PG.v(j);

			umlGraph.x(vG) = drawing.x(PG.copy(vG));
			umlGraph.y(vG) = drawing.y(PG.copy(vG));

			adjEntry adj;
			forall_adj(adj,vG)
			{
				if ((adj->index() & 1) == 0) continue;
				edge eG = adj->theEdge();

				drawing.computePolylineClear(PG,eG,umlGraph.bends(eG));
			}//foralladj
		}//for orig nodes

		if (!umlMerge)
		{
			//insert bend point for incremental mergers
			const SList<node>& mergers = PG.incrementalMergers(i);
			SListConstIterator<node> itMerger = mergers.begin();
			while (itMerger.valid())
			{
				node vMerger = (*itMerger);
				//due to the fact that the merger may be expanded, we are
				//forced to run through the face
				adjEntry adjMerger = PG.expandAdj(vMerger);
				//check if there is an expansion face
				if (adjMerger != 0)
				{
					//we run through the expansion face to the connected edges
					//this edge is to dummy corner
					adjEntry runAdj = adjMerger->faceCycleSucc();
					while (runAdj != adjMerger)
					{
						node vConnect = runAdj->theNode();
						//because of the node collapse using the original
						//edges instead of the merger copy edges (should be
						//fixed for incremental mode) the  degree is 4
						//if (vConnect->degree() != 3)
						if (vConnect->degree() != 4)
						{
							runAdj = runAdj->faceCycleSucc();
							continue;
						}
						edge eCopy = runAdj->cyclicPred()->theEdge();
						OGDF_ASSERT(eCopy->target() == runAdj->theNode())
						OGDF_ASSERT(PG.isGeneralization(eCopy))
						OGDF_ASSERT(PG.original(eCopy))
						umlGraph.bends(PG.original(eCopy)).pushBack(
						DPoint(drawing.x(vMerger), drawing.y(vMerger)));
						runAdj = runAdj->faceCycleSucc();

					}

				}
				else //currently all nodes are expanded, but this is not guaranteed
				{
					forall_adj(adjMerger, vMerger)
					{
						if (adjMerger->theEdge()->target() == vMerger)
						{
							edge eOrig = PG.original(adjMerger->theEdge());
							if (eOrig)
								//incoming merger edges always have an original here!
								umlGraph.bends(eOrig).pushBack(DPoint(drawing.x(vMerger),
								drawing.y(vMerger)));

						}
					}//forall adj
				}
				itMerger++;
			}//while merger nodes
		}

		// the width/height of the layout has been computed by the planar
		// layout algorithm; required as input to packing algorithm
		boundingBox[i] = m_planarLayouter.get().getBoundingBox();
	}//for cc's

	postProcess(umlGraph);

	//----------------------------------------
	// Arrange layouts of connected components
	//----------------------------------------

	arrangeCCs(PG, umlGraph, boundingBox);

	umlGraph.undoGenMergers();
	umlGraph.removeUnnecessaryBendsHV();
}//callfixembed

#endif
//=========================================================


} // end namespace ogdf
