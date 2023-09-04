// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BROWSER_COMMAND_CONTROLLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BROWSER_COMMAND_CONTROLLER_H_

#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"

#define UpdateCommandsForTabStripStateChanged           \
  UpdateCommandsForTabStripStateChanged_Chromium1();    \
                                                        \
 protected:                                             \
  virtual void UpdateCommandsForTabStripStateChanged(); \
                                                        \
 private:                                               \
  void UpdateCommandsForTabStripStateChanged_Chromium2

#include "src/chrome/browser/ui/browser_command_controller.h"

#undef UpdateCommandsForTabStripStateChanged

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BROWSER_COMMAND_CONTROLLER_H_
