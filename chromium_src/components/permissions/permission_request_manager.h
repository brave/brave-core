/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_PERMISSION_REQUEST_MANAGER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_PERMISSION_REQUEST_MANAGER_H_

class WidevinePermissionAndroidTest;
class SplitViewWithTabDialogBrowserTest;

#define set_view_factory_for_testing                                           \
  AcceptDenyCancel(const std::vector<PermissionRequest*>& accepted_requests,   \
                   const std::vector<PermissionRequest*>& denied_requests,     \
                   const std::vector<PermissionRequest*>& cancelled_requests); \
  bool ShouldGroupRequests(PermissionRequest* a, PermissionRequest* b) const;  \
  void OnTabActiveStateChanged(bool active);                                   \
                                                                               \
 private:                                                                      \
  void UpdateTabIsHiddenWithTabActivationState();                              \
  bool ShouldBeGrouppedInRequests(PermissionRequest* a) const;                 \
  friend class ::WidevinePermissionAndroidTest;                                \
  friend class ::SplitViewWithTabDialogBrowserTest;                            \
  bool tab_is_hidden_for_testing() const {                                     \
    return tab_is_hidden_;                                                     \
  }                                                                            \
                                                                               \
 public:                                                                       \
  std::optional<bool> tab_is_activated_;                                       \
  void set_view_factory_for_testing

#include "src/components/permissions/permission_request_manager.h"  // IWYU pragma: export

#undef set_view_factory_for_testing

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_PERMISSION_REQUEST_MANAGER_H_
