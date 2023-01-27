// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// By default the Fuzzy finder doesn't return any results for the empty string.
// However, this isn't ideal because it's not obvious what the input is for.
// It's much better if we can show a bunch of commands.
#define BRAVE_FUZZY_FINDER_NEEDLE_SIZE_CHECK &&false

#include "src/chrome/browser/ui/commander/fuzzy_finder.cc"

#undef BRAVE_FUZZY_FINDER_NEEDLE_SIZE_CHECK
