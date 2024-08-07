// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_TOOLBAR_AI_CHAT_BUTTON_H_
#define BRAVE_BROWSER_UI_VIEWS_TOOLBAR_AI_CHAT_BUTTON_H_

#include "base/memory/raw_ref.h"
#include "chrome/browser/ui/views/toolbar/toolbar_button.h"
#include "ui/base/metadata/metadata_header_macros.h"

class Browser;

class AIChatButton : public ToolbarButton {
  METADATA_HEADER(AIChatButton, ToolbarButton)
 public:
  explicit AIChatButton(Browser* browser);
  AIChatButton(const AIChatButton&) = delete;
  AIChatButton& operator=(const AIChatButton&) = delete;
  ~AIChatButton() override;

 private:
  void ButtonPressed();

  const raw_ref<Browser> browser_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TOOLBAR_AI_CHAT_BUTTON_H_
