/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/brave_channel_info_posix.h"

#include "base/strings/string_util.h"
#include "build/build_config.h"

namespace brave {

version_info::Channel GetChannelImpl(std::string* modifier_out,
                                     std::string* data_dir_suffix_out) {
  version_info::Channel channel = version_info::Channel::UNKNOWN;
  std::string modifier;
  std::string data_dir_suffix;

#if defined(OFFICIAL_BUILD)
  char* env = getenv("CHROME_VERSION_EXTRA");
  if (env)
    modifier = env;

  if (modifier == "unstable")  // linux version of "dev"
    modifier = "dev";
  if (modifier == "stable") {
    channel = version_info::Channel::STABLE;
    modifier = "";
  } else if (modifier == "dev") {
    channel = version_info::Channel::DEV;
    data_dir_suffix = "-Dev";
  } else if (modifier == "beta") {
    channel = version_info::Channel::BETA;
    data_dir_suffix = "-Beta";
  } else {
    modifier = "unknown";
  }
#else
  data_dir_suffix = "-Development";
#endif

  if (modifier_out)
    modifier_out->swap(modifier);
  if (data_dir_suffix_out)
    data_dir_suffix_out->swap(data_dir_suffix);

  return channel;
}

}  // namespace brave
