/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/frame/contents_container_view.h"

// Forward declared to avoid adding a compile-time dependency on Brave's views.
// Implementations are provided by //brave/browser/ui/views/frame/split_view.
//
// These are free functions rather than virtual methods on purpose: the
// constructor below creates these views, and a virtual call during base-class
// construction would silently resolve back to upstream's implementation.
std::unique_ptr<MultiContentsViewMiniToolbar> CreateBraveMiniToolbar(
    BrowserView* browser_view,
    ContentsWebView* contents_view);
std::unique_ptr<ContentsContainerOutline> CreateBraveContainerOutline(
    views::View* mini_toolbar);

#include <chrome/browser/ui/views/frame/contents_container_view.cc>
