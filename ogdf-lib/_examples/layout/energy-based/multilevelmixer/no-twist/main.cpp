#include <ogdf/energybased/multilevelmixer/MMMExampleFastLayout.h>
#include <ogdf/basic/PreprocessorLayout.h>
#include <ogdf/packing/ComponentSplitterLayout.h>
#include <ogdf/energybased/multilevelmixer/ModularMultilevelMixer.h>
#include <ogdf/energybased/multilevelmixer/ScalingLayout.h>
#include <ogdf/energybased/FastMultipoleEmbedder.h>
#include <ogdf/energybased/multilevelmixer/LocalBiconnectedMerger.h>
#include <ogdf/energybased/multilevelmixer/BarycenterPlacer.h>
#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/packing/TileToRowsCCPacker.h>
#include <ogdf/fileformats/GraphIO.h>

using namespace ogdf;

int main(int argc, char **argv) {
	Graph g;
	GraphAttributes ga(g);
	if (!GraphIO::readGML(ga, g, "uk_Pack_Bary_EC_FRENC.gml"))
	{
		cerr << "Could not load Graph" << endl;
		return 1;
	}

	node v;
	forall_nodes(v, g)
	{
		ga.width(v) = ga.height(v) = 10.0;
	}

	MultilevelGraph mlg(ga);

	FastMultipoleEmbedder *fme = new ogdf::FastMultipoleEmbedder();
	fme->setNumIterations(1000);
	fme->setRandomize(false);

	LocalBiconnectedMerger *lbcm = new ogdf::LocalBiconnectedMerger();
	lbcm->setFactor(2.0);
	lbcm->setEdgeLengthAdjustment(0);

	BarycenterPlacer *bp = new ogdf::BarycenterPlacer();
	bp->weightedPositionPriority(true);

	ScalingLayout *sl = new ogdf::ScalingLayout();
	sl->setExtraScalingSteps(1);
	sl->setScaling(5.0, 10.0);
	sl->setScalingType(ogdf::ScalingLayout::st_relativeToDesiredLength);
	sl->setSecondaryLayout(fme);
	sl->setLayoutRepeats(1);

	ModularMultilevelMixer *mmm = new ModularMultilevelMixer;
	mmm->setLayoutRepeats(1);
	mmm->setLevelLayoutModule(sl);
	mmm->setInitialPlacer(bp);
	mmm->setMultilevelBuilder(lbcm);

	TileToRowsCCPacker *ttrccp = new TileToRowsCCPacker;
	ComponentSplitterLayout *cs = new ComponentSplitterLayout;
	cs->setPacker(ttrccp);
	cs->setLayoutModule(mmm);
	PreprocessorLayout ppl;
	ppl.setLayoutModule(cs);
	ppl.setRandomizePositions(true);

	ppl.call(mlg);

	mlg.exportAttributes(ga);

	GraphIO::writeGML(ga, "uk_Pack_Bary_EC_FRENC-mmmnotwist.gml");
}
