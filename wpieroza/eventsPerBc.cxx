#include <DataFormatsFT0/EventsPerBc.h>
#include <Framework/AnalysisTask.h>
#include <Framework/runDataProcessing.h>

struct EventsPerBcGeneration: AnalysisTask
{
  void init()
  {
    
  }
  void process(o2::aod::FT0 const& ft0, o2::aod::BC const bc&)
  {
    std::bitset<8> triggers = ft0.triggerMask();
    bool isVtx = triggers[o2::ft0::Triggers::bitVertex];
    uint32_t bcid = bc.globalBc() % o2::constants::lhc::LHCMaxBunches;
    
    if(isVtx && ft0.sumAmpA() >= mMinAmplitudeSideA && ft0.sumAmpC() >= mMinAmplitudeSideC && ft0.sumAmpA() + ft0.sumAmpC() >= mMinSumOfAmplitude) {
        mCalibrationObjects[bc.runNumber()].histogram[bcid]++;
    }
  }

  ~EventsPerBcGeneration()
  {
      for(const auto& [runNumber, object]: mCalibrationObjects) {
          std::string fileName = mCalibrationObjectFilePrefix + "_" + std::to_string(runNumber);
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

  std::map:<uint32_t, EventsPerBc> mCalibrationObjects;
  int32_t mMinAmplitudeSideA = 5;
  int32_t mMinAmplitudeSideC = 5;
  int32_t mMinSumOfAmplitude = 20;
  std::string mCalibrationObjectFilePrefix = "LHC25ar";
};

WorkflowSpec defineDataProcessing(ConfigContext const &cfgc)
{
    return WorkflowSpec{adaptAnalysisTask<EventsPerBcGeneration>(cfgc)};
}
