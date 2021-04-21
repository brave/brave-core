/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ITEM_CONTROLLER_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ITEM_CONTROLLER_H_

class SidebarItemController {
 public:
  virtual void MaybeStartDrag() = 0;
  virtual void ContinueDrag() = 0;
  virtual void EndDrag() = 0;

 protected:
  ~SidebarItemController() {}
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ITEM_CONTROLLER_H_
