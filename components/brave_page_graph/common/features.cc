/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_page_graph/common/features.h"

namespace brave_page_graph::features {

// Enables PageGraph machinery to generate GraphML representation of web pages
// construction order. Under the hood this enables PageGraph blink CoreProbes
// agent and activates some DevTools APIs to interact with the PageGraph engine.
BASE_FEATURE(kPageGraph, "PageGraph", base::FEATURE_DISABLED_BY_DEFAULT);

}  // namespace brave_page_graph::features
