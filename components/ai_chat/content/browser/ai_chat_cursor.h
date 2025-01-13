// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_AI_CHAT_CURSOR_H_
#define BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_AI_CHAT_CURSOR_H_

#include "base/memory/raw_ptr.h"
#include "ui/views/widget/widget.h"

namespace content {
class WebContents;
}

namespace views {
class ImageView;
}

// A View that displays a fake cursor image above a WebContents.
class AIChatCursorOverlay : public views::View {
    METADATA_HEADER(AIChatCursorOverlay, views::View)
 public:
  // Creates the overlay view and adds it to the same View hierarchy that hosts
  // the given WebContents.
  explicit AIChatCursorOverlay(content::WebContents* web_contents);
  ~AIChatCursorOverlay() override;

  AIChatCursorOverlay(const AIChatCursorOverlay&) = delete;
  AIChatCursorOverlay& operator=(const AIChatCursorOverlay&) = delete;

  void MoveCursorTo(int x, int y);

  void ShowCursor();
  void HideCursor();

 private:
  // The underlying ImageView that draws the cursor graphic.
  raw_ptr<views::ImageView> cursor_image_ = nullptr;
};

#endif  // BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_AI_CHAT_CURSOR_H_
