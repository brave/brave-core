/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_WEB_CONTENTS_DELEGATE_H_
#define BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_WEB_CONTENTS_DELEGATE_H_

#include "content/public/browser/web_contents_delegate.h"

namespace sidebar {

class SidebarWebContentsDelegate : public content::WebContentsDelegate {
 public:
  SidebarWebContentsDelegate();
  ~SidebarWebContentsDelegate() override;

  SidebarWebContentsDelegate(const SidebarWebContentsDelegate&) = delete;
  SidebarWebContentsDelegate& operator=(const SidebarWebContentsDelegate&) =
      delete;
};

}  // namespace sidebar

#endif  // BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_WEB_CONTENTS_DELEGATE_H_
