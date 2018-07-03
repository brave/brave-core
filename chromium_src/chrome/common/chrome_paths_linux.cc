/* This Source Code Form is subject to the terms of the Mozilla Public
+ * License, v. 2.0. If a copy of the MPL was not distributed with this file,
+ * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define GetDefaultUserDataDirectory GetDefaultUserDataDirectory_Disabled
#include "../../../../chrome/common/chrome_paths_linux.cc"
#undef GetDefaultUserDataDirectory

namespace {

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
  if (modifier == "dev") {
    data_dir_suffix = "-Dev";
  } else if (modifier == "beta") {
    data_dir_suffix = "-Beta";
  } else {
    DCHECK(modifier == "stable");
  }

  return data_dir_suffix;
#else  // OFFICIAL_BUILD
  return "-Development";
#endif
}

}  // namespace

namespace chrome {

bool GetDefaultUserDataDirectory(base::FilePath* result) {
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  base::FilePath config_dir;
  std::string chrome_config_home_str;
  config_dir =
      GetXDGDirectory(env.get(), kXdgConfigHomeEnvVar, kDotConfigDir);

  *result = config_dir.Append("BraveSoftware/Brave-Browser" + GetChannelSuffixForDataDir());

  return true;
}

}  // namespace chrome
