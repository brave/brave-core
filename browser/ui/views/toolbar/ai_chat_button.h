// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_TOOLBAR_AI_CHAT_BUTTON_H_
#define BRAVE_BROWSER_UI_VIEWS_TOOLBAR_AI_CHAT_BUTTON_H_

#include <memory>

#include "base/gtest_prod_util.h"
#include "base/memory/raw_ref.h"
#include "chrome/browser/ui/views/toolbar/toolbar_button.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/menus/simple_menu_model.h"

class Browser;
class PrefService;

class AIChatButton : public ToolbarButton,
                     public ui::SimpleMenuModel::Delegate {
  METADATA_HEADER(AIChatButton, ToolbarButton)
 public:
  explicit AIChatButton(Browser* browser);
  AIChatButton(const AIChatButton&) = delete;
  AIChatButton& operator=(const AIChatButton&) = delete;
  ~AIChatButton() override;

 private:
  enum ContextMenuCommand {
    kOpenInFullPage,
    kOpenInSidebar,
    kAboutLeoAI,
    kManageLeoAI,
    kHideAIChatButton,
  };

  FRIEND_TEST_ALL_PREFIXES(BraveToolbarViewTest_AIChatEnabled,
                           AIChatButtonOpenTargetTest);

  void ButtonPressed();
  std::unique_ptr<ui::SimpleMenuModel> CreateMenuModel();

  // ui::SimpleMenuModel::Delegate:
  void ExecuteCommand(int command_id, int event_flags) override;
  bool IsCommandIdChecked(int command_id) const override;

  const raw_ref<Browser> browser_;
  raw_ref<PrefService> prefs_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TOOLBAR_AI_CHAT_BUTTON_H_
