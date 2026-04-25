// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ui/color/color_provider_key.h"

// Add switch-case handling for kDarker case. Use the same Config of kTonalSpot.
#define kTonalSpot \
  kDarker:         \
  case SchemeVariant::kTonalSpot

#include <ui/color/dynamic_color/palette_factory.cc>

#undef kTonalSpot
