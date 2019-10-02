/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_BRAVE_HISTOGRAM_REWRITE_H_
#define BRAVE_COMPONENTS_P3A_BRAVE_HISTOGRAM_REWRITE_H_

namespace brave {

// Set callbacks for existing Chromium histograms that will be braveized,
// i.e. reemitted using a different name and custom buckets.
void SetupHistogramsBraveization();

}  // namespace brave

#endif  // BRAVE_COMPONENTS_P3A_BRAVE_HISTOGRAM_REWRITE_H_
