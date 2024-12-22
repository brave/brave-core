// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_AI_CHAT_CURSOR_H_
#define BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_AI_CHAT_CURSOR_H_

#include "base/memory/raw_ptr.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/views/widget/widget.h"

namespace content {
class WebContents;
}

namespace views {
class ImageView;
}


// A View that displays a "fake cursor" image above a given WebContents.
// It’s added as a child of the WebContents’ root view, so it moves with it.
class AIChatCursorOverlay : public views::View {
    METADATA_HEADER(AIChatCursorOverlay, views::View)
 public:
  // Creates the overlay view and adds it to the same View hierarchy that hosts
  // the given WebContents.
  explicit AIChatCursorOverlay(content::WebContents* web_contents);
  ~AIChatCursorOverlay() override;

  AIChatCursorOverlay(const AIChatCursorOverlay&) = delete;
  AIChatCursorOverlay& operator=(const AIChatCursorOverlay&) = delete;

  // Moves the fake cursor image to the specified position (x,y) in the parent
  // view’s coordinate space (typically the top-left corner of the web contents).
  void MoveCursorTo(int x, int y);

  // Show/hide the fake cursor. When hidden, the ImageView is simply invisible.
  void ShowCursor();
  void HideCursor();

 private:
  // Overridden from views::View:
  void OnBoundsChanged(const gfx::Rect& previous_bounds) override;

  // The underlying ImageView that draws the cursor graphic.
  raw_ptr<views::ImageView> cursor_image_ = nullptr;
};

// // A class that creates an overlay widget on top of a given WebContents,
// // showing a fake mouse cursor image. You can show, hide, and move it.
// class AIChatCursorOverlay {
//  public:
//   explicit AIChatCursorOverlay(content::WebContents* web_contents);
//   ~AIChatCursorOverlay();

//   AIChatCursorOverlay(const AIChatCursorOverlay&) = delete;
//   AIChatCursorOverlay& operator=(const AIChatCursorOverlay&) = delete;

//   // Shows the fake cursor overlay if not already visible.
//   void Show();

//   // Hides the fake cursor overlay if it is visible.
//   void Hide();

//   // Moves the fake cursor overlay to the given coordinates, relative to the
//   // WebContents’ view coordinate space.
//   void MoveCursorTo(int x, int y);

//  private:
//   // Creates and configures the widget if not already created.
//   void InitializeWidget();

//   raw_ptr<views::Widget> widget_ = nullptr;
//   raw_ptr<views::ImageView> image_view_ = nullptr;
//   gfx::NativeView parent_window_;
//   bool initialized_ = false;
// };

#endif  // BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_AI_CHAT_CURSOR_H_
