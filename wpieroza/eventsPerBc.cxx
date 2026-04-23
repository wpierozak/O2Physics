#include <array>
#include <DataFormatsFT0/EventsPerBc.h>
#include <DataFormatsFIT/Triggers.h>
#include <Framework/AnalysisTask.h>
#include <Framework/runDataProcessing.h>
#include <Framework/AnalysisHelpers.h>
#include <TFile.h>
#include <CCDB/CcdbApi.h>
#include <CommonConstants/LHCConstants.h>
#include <Framework/Logger.h>
#include <Common/DataModel/EventSelection.h>
#include <Common/DataModel/FT0Corrected.h>
#include <Common/DataModel/Multiplicity.h>
#include <Framework/EndOfStreamContext.h>
#include <DataFormatsFT0/Digit.h>

#include <Framework/HistogramRegistry.h>
#include <Framework/HistogramSpec.h>
#include <Framework/OutputObjHeader.h>
using namespace o2;
using namespace o2::framework;

using BCsWithRun3Matchings = o2::soa::Join<o2::aod::BCs, o2::aod::BcSels, o2::aod::Timestamps, o2::aod::Run3MatchedToBCSparse>;

struct EventsPerBcGeneration
{
  o2::framework::Configurable<int32_t> configMinAmplitudeSideA{"minAmplitudeSideA", 5, "minimum amplitude side A"};
  o2::framework::Configurable<int32_t> configMinAmplitudeSideC{"minAmplitudeSideC", 5, "minimum amplitude side C"};
  o2::framework::Configurable<int32_t> configMinSumOfAmplitude{"minSumOfAmplitude", 20, "minimum sum of amplitudes from both sides"};
  o2::framework::Configurable<std::string> configCalibrationObjectFilePrefix{"objectFileNamePrefix", "ft0EventsPerBc_", "prefix of object file"};

  framework::HistogramRegistry histograms{"Histograms", {}};
  void init(o2::framework::InitContext const& ic)
  {
    const AxisSpec axisBcs(3564, 0, 3563, "bunch collision");
    histograms.add("hFT0_events_per_bc", "FT0 Events Per BC", kTH1F, {axisBcs});
    mMinAmplitudeSideA = configMinAmplitudeSideA;
    mMinAmplitudeSideC = configMinAmplitudeSideC;
    mMinSumOfAmplitude = configMinSumOfAmplitude;
    LOGP(info, "Amplitude thresholds: side A - {}, side C - {}, sum - {}", mMinAmplitudeSideA, mMinAmplitudeSideC, mMinSumOfAmplitude);
  }

  void process(BCsWithRun3Matchings const& bcs, aod::FT0s const&)
  {
      for(const auto& bc: bcs) {
        if(bc.has_ft0() == false) {
            continue;
        }
        LOGP(debug, "Processing BC with FT0 entry: {}", bc.globalBC());
        auto ft0 = bc.ft0();
        std::bitset<8> triggers = ft0.triggerMask();
        bool isVtx = triggers[ft0::Triggers::bitVertex];
        uint32_t bcid = bc.globalBC() % o2::constants::lhc::LHCMaxBunches;
        
        if(isVtx && ft0.sumAmpA() >= mMinAmplitudeSideA && ft0.sumAmpC() >= mMinAmplitudeSideC 
                && ft0.sumAmpA() + ft0.sumAmpC() >= mMinSumOfAmplitude) {
            LOGP(debug, "Accepted event: BC - {}; amplitude A - {}; amplitude C - {}; amplitude Sum - {}", 
                    bcid, ft0.sumAmpA(), ft0.sumAmpC(), ft0.sumAmpA() + ft0.sumAmpC());
            histograms.fill(HIST("hFT0_events_per_bc"), bcid);
        } else {
             LOGP(debug, "Discarded event: BC - {}; amplitude A - {}; amplitude C - {}; amplitude Sum - {}", 
                    bcid, ft0.sumAmpA(), ft0.sumAmpC(), ft0.sumAmpA() + ft0.sumAmpC());
        }
    
     }
  }

  const uint64_t ctpTVX = (1ull << 2);

//  framework::Service<EventPerBcContainer> mCalibrationContainer;
  int32_t mMinAmplitudeSideA;
  int32_t mMinAmplitudeSideC;
  int32_t mMinSumOfAmplitude;
  std::string mCalibrationObjectFilePrefix;
};

o2::framework::WorkflowSpec defineDataProcessing(o2::framework::ConfigContext const &cfgc)
{
    return o2::framework::WorkflowSpec{adaptAnalysisTask<EventsPerBcGeneration>(cfgc)};
}
