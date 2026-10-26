// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
#include "chrome/browser/ui/views/context_hub/save_to_memory_bank_bubble_view.h"

#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "ui/views/layout/layout_provider.h"

// Use Brave's bubble radius instead of Emphasis::kHigh. See
// ShapeContextTokensOverride::kHigh.
#define Emphasis ShapeContextTokensOverride
#include <chrome/browser/ui/views/context_hub/save_to_memory_bank_bubble_view.cc>  // IWYU pragma: keep
#undef Emphasis
