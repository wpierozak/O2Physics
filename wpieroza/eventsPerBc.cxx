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

using namespace o2;

using BCsWithRun3Matchings = o2::soa::Join<o2::aod::BCs, o2::aod::BcSels, o2::aod::Timestamps, o2::aod::Run3MatchedToBCSparse>;


struct EventPerBcContainer
{
   static EventPerBcContainer& instance()
   {
    static EventPerBcContainer container;  
    return container;
   }

   std::map<uint32_t, o2::ft0::EventsPerBc> mCalibrationObjects;
   std::string mCalibrationObjectFilePrefix;

   void endOfStream()
   {
      LOGP(info, "Calibration objects to save: {}", mCalibrationObjects.size());
      for(const auto& [runNumber, object]: mCalibrationObjects) {
          std::string fileName = mCalibrationObjectFilePrefix + std::to_string(runNumber);
          try {
            TFile fout(fileName.c_str(), "recreate");
            fout.WriteObjectAny(&object, "o2::ft0::EventsPerBc", o2::ccdb::CcdbApi::CCDBOBJECT_ENTRY);
            fout.Close();
            LOGP(info, "Saved calibration object for run {} to file {}", runNumber, fileName);
          }
          catch(const std::exception& ex) {
              LOGP(error, "Failed to store calibration object for run {} to file {}", runNumber, fileName);;
          }
          
      }   
   }
};

struct EventsPerBcGeneration
{
  o2::framework::Configurable<int32_t> configMinAmplitudeSideA{"minAmplitudeSideA", 5, "minimum amplitude side A"};
  o2::framework::Configurable<int32_t> configMinAmplitudeSideC{"minAmplitudeSideC", 5, "minimum amplitude side C"};
  o2::framework::Configurable<int32_t> configMinSumOfAmplitude{"minSumOfAmplitude", 20, "minimum sum of amplitudes from both sides"};
  o2::framework::Configurable<std::string> configCalibrationObjectFilePrefix{"objectFileNamePrefix", "ft0EventsPerBc_", "prefix of object file"};

  void init(o2::framework::InitContext const& ic)
  {
    mMinAmplitudeSideA = configMinAmplitudeSideA;
    mMinAmplitudeSideC = configMinAmplitudeSideC;
    mMinSumOfAmplitude = configMinSumOfAmplitude;
    mCalibrationContainer->mCalibrationObjectFilePrefix = configCalibrationObjectFilePrefix;
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
            mCalibrationContainer->mCalibrationObjects[bc.runNumber()].histogram[bcid]++;
        } else {
             LOGP(debug, "Discarded event: BC - {}; amplitude A - {}; amplitude C - {}; amplitude Sum - {}", 
                    bcid, ft0.sumAmpA(), ft0.sumAmpC(), ft0.sumAmpA() + ft0.sumAmpC());
        }
    
     }
  }

  const uint64_t ctpTVX = (1ull << 2);

  framework::Service<EventPerBcContainer> mCalibrationContainer;
  int32_t mMinAmplitudeSideA;
  int32_t mMinAmplitudeSideC;
  int32_t mMinSumOfAmplitude;
  std::string mCalibrationObjectFilePrefix;
};

o2::framework::WorkflowSpec defineDataProcessing(o2::framework::ConfigContext const &cfgc)
{
    return o2::framework::WorkflowSpec{adaptAnalysisTask<EventsPerBcGeneration>(cfgc)};
}
