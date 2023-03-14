/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_BUBBLE_WEBUI_BUBBLE_DIALOG_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_BUBBLE_WEBUI_BUBBLE_DIALOG_VIEW_H_

#include <memory>

#include "chrome/browser/ui/views/bubble/bubble_contents_wrapper.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/controls/menu/menu_runner.h"
#include "ui/webui/mojo_bubble_web_ui_controller.h"

#define ShowUI                                                               \
  ShowCustomContextMenu(gfx::Point point,                                    \
                        std::unique_ptr<ui::MenuModel> menu_model) override; \
  void HideCustomContextMenu() override;                                     \
  void ShowUI

#define bubble_anchor_                                     \
  bubble_anchor_;                                          \
  std::unique_ptr<views::MenuRunner> context_menu_runner_; \
  std::unique_ptr<ui::MenuModel> context_menu_model_

#include "src/chrome/browser/ui/views/bubble/webui_bubble_dialog_view.h"  // IWYU pragma: export
#undef bubble_anchor_
#undef ShowUI

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_BUBBLE_WEBUI_BUBBLE_DIALOG_VIEW_H_
