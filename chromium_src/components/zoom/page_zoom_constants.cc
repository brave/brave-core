/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/zoom/page_zoom_constants.h"

#define kPresetZoomFactors kPresetZoomFactors_ChromiumImpl
#define kPresetZoomFactorsSize kPresetZoomFactorsSize_ChromiumImpl
#include "../../../../components/zoom/page_zoom_constants.cc"
#undef kPresetZoomFactors
#undef kPresetZoomFactorsSize

namespace zoom {

const double kPresetZoomFactors[] = { 0.25, 1 / 3.0, 0.5, 2 / 3.0, 0.75, 0.8,
                                      0.9, 1.0, 1.1, 1.25, 4 / 3.0, 7 / 5.0,
                                      1.5, 1.75, 2.0, 2.5, 3.0, 4.0, 5.0};
const std::size_t kPresetZoomFactorsSize = base::size(kPresetZoomFactors);

}  // namespace zoom
