// Copyright (c) 2020 The Brave Authors
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "base/strings/string_util.h"
#include "build/build_config.h"

#define STABLE_DISPLAY_NAME "brave-browser"
#define DEV_DISPLAY_NAME "brave-browser-dev"
#define BETA_DISPLAY_NAME "brave-browser-beta"
#define NIGHTLY_DISPLAY_NAME "brave-browser-nightly"

namespace media {

namespace pulse {

namespace {
    // We can't use brave::GetChannelName() because audio is a dependency of
    // common, so we implement minimal channel checking here to determine which
    // icon to use. This is Linux only so it's fine just using those modifiers.
    //
    // The defines are pulled in via a very small patch to the audio BUILD.gn.
    const char* BrowserIconName() {
#if defined(OFFICIAL_BUILD)
      std::string modifier;
      char* env = getenv("CHROME_VERSION_EXTRA");
      if (env)
        modifier = env;

      if (modifier == LINUX_CHANNEL_STABLE) {
        return STABLE_DISPLAY_NAME;
      } else if (modifier == LINUX_CHANNEL_BETA) {
        return BETA_DISPLAY_NAME;
      } else if (modifier == BRAVE_LINUX_CHANNEL_NIGHTLY) {
        return NIGHTLY_DISPLAY_NAME;
      } else {
        return DEV_DISPLAY_NAME;
      }
#else
      return DEV_DISPLAY_NAME;
#endif  // defined(OFFICIAL_BUILD)
    }
}  // anonymous namespace
}  // namespace pulse
}  // namespace media

#define PRODUCT_STRING "Brave"
#include "../../../../../media/audio/pulse/pulse_util.cc"
#undef PRODUCT_STRING
