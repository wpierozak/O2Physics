#include <DataFormatsFT0/EventsPerBc.h>
#include <Framework/AnalysisTask.h>
#include <Framework/runDataProcessing.h>

struct EventsPerBcGeneration: o2::framework::AnalysisTask
{
  o2::framework::Configurable<int32_t> configMinAmplitudeSideA("minAmplitudeSideA", 5, "minimum amplitude side A");
  o2::framework::Configurable<int32_t> configMinAmplitudeSideC("minAmplitudeSideC", 5, "minimum amplitude side C");
  o2::framework::Configurable<int32_t> configMinSumOfAmplitude("minSumOfAmplitude", 20, "minimum sum of amplitudes from both sides");
  o2::framework::Configurable<std::string> configCalibrationObjectFilePrefix("objectFileNamePrefix", "ft0EventsPerBc_", "prefix of object file")
  void init(o2::framework::InitContext& const&)
  {
    mMinAmplitudeSideA = configMinAmplitudeSideA;
    mMinAmplitudeSideC = configMinAmplitudeSideC;
    mMinSumOfAmplitude = configMinSumOfAmplitude;
    mCalibrationObjectFilePrefix = configCalibrationObjectFilePrefix;
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

  std::map:<uint32_t, EventsPerBc> mCalibrationObjects;
  int32_t mMinAmplitudeSideA;
  int32_t mMinAmplitudeSideC;
  int32_t mMinSumOfAmplitude;
  std::string mCalibrationObjectFilePrefix;
};

WorkflowSpec defineDataProcessing(ConfigContext const &cfgc)
{
    return WorkflowSpec{adaptAnalysisTask<EventsPerBcGeneration>(cfgc)};
}
