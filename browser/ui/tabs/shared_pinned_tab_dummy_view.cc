// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/tabs/shared_pinned_tab_dummy_view.h"

SharedPinnedTabDummyView::SharedPinnedTabDummyView(
    content::WebContents* shared_contents,
    content::WebContents* dummy_contents)
    : shared_contents_(shared_contents), dummy_contents_(dummy_contents) {}

SharedPinnedTabDummyView::~SharedPinnedTabDummyView() = default;
