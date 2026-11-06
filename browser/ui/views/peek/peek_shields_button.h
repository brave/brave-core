/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PEEK_PEEK_SHIELDS_BUTTON_H_
#define BRAVE_BROWSER_UI_VIEWS_PEEK_PEEK_SHIELDS_BUTTON_H_

#include <string>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/controls/button/label_button.h"

class BrowserWindowInterface;

namespace content {
class WebContents;
}  // namespace content

// Shows whether Shields is on for the Peek page. Pressing it runs `callback`,
// which moves the page into a tab and opens the Shields panel there: changing
// Shields is a decision about the site, made with the tab's full browser UI.
class PeekShieldsButton : public views::LabelButton {
  METADATA_HEADER(PeekShieldsButton, views::LabelButton)

 public:
  PeekShieldsButton(BrowserWindowInterface& browser,
                    views::Button::PressedCallback callback);
  PeekShieldsButton(const PeekShieldsButton&) = delete;
  PeekShieldsButton& operator=(const PeekShieldsButton&) = delete;
  ~PeekShieldsButton() override;

  void SetWebContents(content::WebContents* contents);
  void UpdateImage();

  // views::LabelButton:
  void OnThemeChanged() override;

 private:
  const raw_ref<BrowserWindowInterface> browser_;
  base::WeakPtr<content::WebContents> web_contents_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PEEK_PEEK_SHIELDS_BUTTON_H_
