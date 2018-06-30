/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/brave_channel_info_linux.h"

namespace brave {

std::string GetChannelSuffixForDataDir() {
#if defined(OFFICIAL_BUILD)
  std::string modifier;
  std::string data_dir_suffix;

  char* env = getenv("CHROME_VERSION_EXTRA");
  if (env)
    modifier = env;

  // Chrome doesn't support canary channel on linux.
  if (modifier == "unstable")  // linux version of "dev"
    modifier = "dev";
  if (modifier == "dev")
    data_dir_suffix = "-Dev";
  else if (modifier == "beta")
    data_dir_suffix = "-Beta";
  else if (modifier == "stable")
    data_dir_suffix = "";

  return data_dir_suffix;
#else
  return "-Development";
#endif
}

}  // namespace brave
