/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/page_info/brave_page_info_bubble_view.h"

// `PageInfoBubbleView::CreatePageInfoBubble` is patched to create an instance
// of `BravePageInfoBubbleView`. Direct patching is used to ensure that the
// subclass is only referenced where needed.

#include <chrome/browser/ui/views/page_info/page_info_bubble_view.cc>
