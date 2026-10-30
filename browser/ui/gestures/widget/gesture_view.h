// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_GESTURES_WIDGET_GESTURE_VIEW_H_
#define BRAVE_BROWSER_UI_GESTURES_WIDGET_GESTURE_VIEW_H_

#include <string>

#include "brave/components/gestures/core/common/gesture_path_types.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/view.h"

namespace gfx {
class Canvas;
}  // namespace gfx

namespace gestures {

// Paints a fading trail following the cursor and a label naming the
// currently-matched command, inside `GestureWidget`.
class GestureView final : public views::View {
  METADATA_HEADER(GestureView, views::View)

 public:
  GestureView();

  GestureView(const GestureView&) = delete;
  GestureView& operator=(const GestureView&) = delete;

  ~GestureView() override;

  void SetTrail(bool show_trail,
                const GesturePath& gesture_path,
                const std::u16string& label);

  // views::View:
  void OnPaint(gfx::Canvas* canvas) override;

 private:
  void DrawTrail(gfx::Canvas& canvas);
  void DrawLabel(gfx::Canvas& canvas);

  bool show_trail_ = false;
  GesturePath gesture_path_;
  std::u16string label_;
};

}  // namespace gestures

#endif  // BRAVE_BROWSER_UI_GESTURES_WIDGET_GESTURE_VIEW_H_
