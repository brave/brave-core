/* This Source Code Form is subject to the terms of the Mozilla Public
+ * License, v. 2.0. If a copy of the MPL was not distributed with this file,
+ * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_browser_main_extra_parts_views_linux.h"

#include "brave/browser/ui/views/brave_views_delegate_linux.h"

BraveBrowserMainExtraPartsViewsLinux::BraveBrowserMainExtraPartsViewsLinux() {
}

BraveBrowserMainExtraPartsViewsLinux::~BraveBrowserMainExtraPartsViewsLinux() {
}

void BraveBrowserMainExtraPartsViewsLinux::ToolkitInitialized() {
  if (!views::ViewsDelegate::GetInstance())
    views_delegate_ = std::make_unique<BraveViewsDelegateLinux>();

  ChromeBrowserMainExtraPartsViewsLinux::ToolkitInitialized();
}

