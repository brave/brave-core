/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/strings/grit/components_strings.h"

namespace {
constexpr int kDangerousVerboseState = IDS_DANGEROUS_VERBOSE_STATE;
}  // namespace

// To make |is_text_dangerous| false always.
// It prevents to get color from LocationIconView. We want to get color
// from its delegate.
#undef IDS_DANGEROUS_VERBOSE_STATE
#define IDS_DANGEROUS_VERBOSE_STATE kDangerousVerboseState) && (false

#include "src/chrome/browser/ui/views/location_bar/location_icon_view.cc"

#undef IDS_DANGEROUS_VERBOSE_STATE

#define IDS_DANGEROUS_VERBOSE_STATE kDangerousVerboseState
