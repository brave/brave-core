// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_BRAVE_BROWSER_COMMAND_CONTROLLER_H_
#define BRAVE_BROWSER_UI_BRAVE_BROWSER_COMMAND_CONTROLLER_H_

#include "chrome/browser/ui/browser_command_controller.h"

class Browser;

class BraveBrowserCommandController : public BrowserCommandController {
 public:
  explicit BraveBrowserCommandController(Browser* browser);
  ~BraveBrowserCommandController() override;

  // BrowserCommandController overrides:
  bool ExecuteCommand(int id, int event_flags) override;
  void InitCommandState() override;

 private:
  // Handles Brave-specific commands
  bool ExecuteBraveCommand(int id, int event_flags);
  
  DISALLOW_COPY_AND_ASSIGN(BraveBrowserCommandController);
};

#endif  // BRAVE_BROWSER_UI_BRAVE_BROWSER_COMMAND_CONTROLLER_H_