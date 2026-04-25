// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/variations/command_line_utils.h"

#include <string>

#include "base/command_line.h"
#include "base/strings/string_util.h"
#include "brave/components/variations/buildflags.h"
#include "brave/components/variations/switches.h"
#include "components/variations/variations_switches.h"

namespace variations {

namespace {

// A GitHub workflow in the brave/brave-variations repository generates the test
// seed and uploads it to a URL with the following template, where $1 is the
// pull request number.
constexpr char kVariationsPrTestSeedUrlTemplate[] =
    "https://griffin.brave.com/pull/$1/seed";

}  // namespace

void AppendBraveCommandLineOptions(base::CommandLine& command_line) {
  std::string variations_server_url = BUILDFLAG(BRAVE_VARIATIONS_SERVER_URL);

  if (command_line.HasSwitch(switches::kVariationsPR)) {
    variations_server_url = base::ReplaceStringPlaceholders(
        kVariationsPrTestSeedUrlTemplate,
        {command_line.GetSwitchValueASCII(switches::kVariationsPR)}, nullptr);

    // Generated seed is not signed, so we need to disable signature check.
    command_line.AppendSwitch(
        variations::switches::kAcceptEmptySeedSignatureForTesting);

    // Disable fetch throttling to force the fetch at startup on mobile
    // platforms.
    command_line.AppendSwitch(
        variations::switches::kDisableVariationsSeedFetchThrottling);
  }

  if (!command_line.HasSwitch(variations::switches::kVariationsServerURL)) {
    command_line.AppendSwitchASCII(variations::switches::kVariationsServerURL,
                                   variations_server_url);
  }

  // Insecure fall-back for variations is set to the same (secure) URL. This
  // is done so that if VariationsService tries to fall back to insecure url
  // the check for kHttpScheme in VariationsService::MaybeRetryOverHTTP would
  // prevent it from doing so as we don't want to use an insecure fall-back.
  if (!command_line.HasSwitch(
          variations::switches::kVariationsInsecureServerURL)) {
    command_line.AppendSwitchASCII(
        variations::switches::kVariationsInsecureServerURL,
        variations_server_url);
  }
}

}  // namespace variations
