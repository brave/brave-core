#define GetEffectiveAutoplayPolicy GetEffectiveAutoplayPolicy_ChromiumImpl
#include "../../../../media/base/media_switches.cc"
#undef GetEffectiveAutoplayPolicy

namespace media {

MEDIA_EXPORT
std::string GetEffectiveAutoplayPolicy(const base::CommandLine& command_line) {
  // Return the autoplay policy set in the command line, if any.
  if (!command_line.HasSwitch(switches::kAutoplayPolicy) &&
      base::FeatureList::IsEnabled(media::kUnifiedAutoplay))
    return switches::autoplay::kUserGestureRequiredPolicy;

  return GetEffectiveAutoplayPolicy_ChromiumImpl(command_line);
}

}  // namespace media
