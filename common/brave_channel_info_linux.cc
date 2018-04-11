/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/brave_channel_info_linux.h"

namespace brave {

std::string GetChannelSuffixForDataDir() {
  std::string modifier;
  std::string data_dir_suffix;

  char* env = getenv("CHROME_VERSION_EXTRA");
  if (env)
    modifier = env;

  // Chrome doesn't support canary channel on linux.
  if (modifier == "unstable")  // linux version of "dev"
    data_dir_suffix = "-Dev";
  else if (modifier == "beta")
    data_dir_suffix = "-Beta";

  return data_dir_suffix;
}

}  // namespace brave
