/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/omnibox/brave_omnibox_result_view.h"
#include "brave/browser/ui/views/omnibox/brave_rounded_omnibox_results_frame.h"

#define RoundedOmniboxResultsFrame BraveRoundedOmniboxResultsFrame
#include "src/chrome/browser/ui/views/omnibox/omnibox_popup_view_views.cc"
#undef RoundedOmniboxResultsFrame
