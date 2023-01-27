// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/commander/common/features.h"

// We replace Chromium's flag with one of our own, because we want it to be
// accessible from //components (which //chrome/ui/browser is not).
#define kQuickCommands kBraveCommander
#include "src/chrome/browser/ui/commander/commander.cc"
#undef kQuickCommands
