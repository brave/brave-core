/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SIDEBAR_SIDEBAR_SERVICE_DELEGATE_H_
#define BRAVE_COMPONENTS_SIDEBAR_SIDEBAR_SERVICE_DELEGATE_H_

namespace sidebar {

class SidebarServiceDelegate {
 public:
  SidebarServiceDelegate() = default;
  SidebarServiceDelegate(const SidebarServiceDelegate&) = delete;
  SidebarServiceDelegate& operator=(const SidebarServiceDelegate&) = delete;
  virtual ~SidebarServiceDelegate() = default;

  virtual void MoveSidebarToRightTemporarily() = 0;
  virtual void RestoreSidebarAlignmentIfNeeded() = 0;
};

}  // namespace sidebar

#endif  // BRAVE_COMPONENTS_SIDEBAR_SIDEBAR_SERVICE_DELEGATE_H_
