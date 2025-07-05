// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/component_updater/brave_component_contents_verifier.h"
#include "services/network/public/cpp/network_switches.h"

#define kHostResolverRules \
  kHostResolverRules, component_updater::kBypassComponentContentsVerifier

#include "src/chrome/browser/ui/startup/bad_flags_prompt.cc"

#undef kHostResolverRules
