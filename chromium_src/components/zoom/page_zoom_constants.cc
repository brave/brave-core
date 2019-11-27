// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/zoom/page_zoom_constants.h"

#include "base/stl_util.h"

namespace zoom {

// This list is duplicated in chrome/browser/resources/pdf/viewport.js and in
// chrome/browser/resources/settings/appearance_page/appearance_page.js. Please
// make sure the three match.
const double kPresetZoomFactors[] = {0.25, 1 / 3.0, 0.5, 2 / 3.0, 0.75, 0.8,
                                     0.9, 1.0, 1.1, 1.25, 4 / 3.0, 7 / 5.0,
                                     1.5, 1.75, 2.0, 2.5,
                                     3.0, 4.0, 5.0};
const std::size_t kPresetZoomFactorsSize = base::size(kPresetZoomFactors);

}  // namespace ui_zoom

