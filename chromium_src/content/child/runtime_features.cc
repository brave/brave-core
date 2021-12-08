#define SetRuntimeFeaturesDefaultsAndUpdateFromArgs SetRuntimeFeaturesDefaultsAndUpdateFromArgs_ChromiumImpl
#include "src/content/child/runtime_features.cc"
#undef SetRuntimeFeaturesDefaultsAndUpdateFromArgs

namespace content {

void SetRuntimeFeaturesDefaultsAndUpdateFromArgs(
    const base::CommandLine& command_line) {
  SetRuntimeFeaturesDefaultsAndUpdateFromArgs_ChromiumImpl(command_line);
}

} // namespace content
