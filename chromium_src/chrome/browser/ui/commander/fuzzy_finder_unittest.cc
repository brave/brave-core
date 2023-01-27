// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Disabled, because we want the empty string to match - it's weird if nothing
// shows up.
#define EmptyStringDoesNotMatch DISABLED_EmptyStringDoesNotMatch

#include "src/chrome/browser/ui/commander/fuzzy_finder_unittest.cc"

#undef EmptyStringDoesNotMatch
