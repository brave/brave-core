// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/metrics/histogram_functions.h"

// Scrubs out the histogramming overload for UmaHistogramEnumeration to
// avoid crash from Brave-specific commands.
#define UmaHistogramEnumeration(...) DoNothing()

#include <chrome/browser/ui/views/download/download_ui_context_menu_view.cc>

#undef UmaHistogramEnumeration
