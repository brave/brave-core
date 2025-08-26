// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <chrome/browser/devtools/devtools_ui_controller.cc>

void DevtoolsUIController::MakeSureControllerExists(
    ContentsContainerView* view) {
  if (devtools_web_view_controllers_.find(view) ==
      devtools_web_view_controllers_.end()) {
    devtools_web_view_controllers_[view] =
        std::make_unique<DevtoolsWebViewController>(view);
  }
}
