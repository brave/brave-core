#include "chrome/common/channel_info.h"

#include "build/build_config.h"
#include "components/version_info/version_info.h"

namespace chrome {

version_info::Channel GetChannel() {
  return version_info::Channel::STABLE;
}

}  // namespace chrome
