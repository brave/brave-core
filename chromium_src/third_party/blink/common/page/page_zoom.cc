/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/blink/public/common/page/page_zoom.h"

#define kPresetBrowserZoomFactorsArray \
  kPresetBrowserZoomFactorsArray_ChromiumImpl
#define kPresetBrowserZoomFactors kPresetBrowserZoomFactors_ChromiumImpl
#include "src/third_party/blink/common/page/page_zoom.cc"
#undef kPresetBrowserZoomFactorsArray
#undef kPresetBrowserZoomFactors

namespace blink {

static constexpr double kPresetBrowserZoomFactorsArray[] = {
    0.25,    1 / 3.0, 0.5, 2 / 3.0, 0.75, 0.8, 0.9, 1.0, 1.1, 1.25,
    4 / 3.0, 7 / 5.0, 1.5, 1.75,    2.0,  2.5, 3.0, 4.0, 5.0};
const base::span<const double> kPresetBrowserZoomFactors(
    kPresetBrowserZoomFactorsArray);

}  // namespace blink
