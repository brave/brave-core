// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/webui/side_panel/customize_chrome/customize_toolbar/customize_toolbar_handler.h"

#include "brave/browser/ui/webui/side_panel/customize_chrome/customize_toolbar/list_action_modifiers.h"

#define ListActions(...) ListActionsChromium(__VA_ARGS__)

#include "src/chrome/browser/ui/webui/side_panel/customize_chrome/customize_toolbar/customize_toolbar_handler.cc"

#undef ListActions

void CustomizeToolbarHandler::ListActions(ListActionsCallback callback) {
  ListActionsChromium(
      base::BindOnce(&customize_chrome::FilterUnsupportedChromiumActions)
          .Then(std::move(callback)));
}
