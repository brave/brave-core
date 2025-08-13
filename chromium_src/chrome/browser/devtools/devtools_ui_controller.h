// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef DEVTOOLS_UI_CONTROLLER_H_
#define DEVTOOLS_UI_CONTROLLER_H_

// Add method to make sure controller exists for the given view.
// This is neede because we can't pass split view's secondary
// ContentsContainerView at the construction time of DevtoolsUIController.
#define TearDown                                              \
  TearDown_Unused();                                          \
  void MakeSureControllerExists(ContentsContainerView* view); \
  void TearDown

#include <chrome/browser/devtools/devtools_ui_controller.h>  // IWYU pragma: export

#undef TearDown

#endif  // DEVTOOLS_UI_CONTROLLER_H_
