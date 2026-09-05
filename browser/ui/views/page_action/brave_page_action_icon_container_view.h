/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_BRAVE_PAGE_ACTION_ICON_CONTAINER_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_BRAVE_PAGE_ACTION_ICON_CONTAINER_VIEW_H_

#include "chrome/browser/ui/views/page_action/page_action_icon_container.h"

class BravePageActionIconContainerView : public PageActionIconContainerView {
  METADATA_HEADER(BravePageActionIconContainerView, PageActionIconContainerView)
 public:

  // Unfortunately, |PageActionIconParams| can't not be copied or moved. So drop
  // const qualifier to modify |param| for Brave.
  explicit BravePageActionIconContainerView(PageActionIconParams& param);
  ~BravePageActionIconContainerView() override;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_BRAVE_PAGE_ACTION_ICON_CONTAINER_VIEW_H_
