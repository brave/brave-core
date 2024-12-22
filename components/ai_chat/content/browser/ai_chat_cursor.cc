// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/content/browser/ai_chat_cursor.h"

#include "base/check.h"
#include "brave/grit/brave_theme_resources.h"
// TODO(petemill): Obviously this isn't allowed here, this class will be moved
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "content/public/browser/web_contents.h"
#include "include/core/SkColor.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/compositor/scoped_layer_animation_settings.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/background.h"
#include "ui/compositor/layer.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

namespace {

#define FAKE_CURSOR_RESOURCE_ID IDR_MY_FAKE_CURSOR_ICON

}  // namespace

AIChatCursorOverlay::AIChatCursorOverlay(content::WebContents* web_contents) {
  DCHECK(web_contents);

  auto* root_view = chrome::FindBrowserWithTab(web_contents)->GetBrowserView().contents_container();//.GetContentsWebView()->parent();
  DCHECK(root_view);

  cursor_image_ = AddChildView(std::make_unique<views::ImageView>());
  SetPaintToLayer();
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  gfx::ImageSkia* image = rb.GetImageSkiaNamed(IDR_SIDEBAR_READING_LIST_PANEL_HEADER);
  SetBackground(views::CreateSolidBackground(SK_ColorBLUE));
  DCHECK(image);
  cursor_image_->SetImage(*image);

  gfx::Size image_size = cursor_image_->GetPreferredSize();

  // SetBoundsRect(gfx::Rect(0, 0, 500, 500));
  SetBoundsRect(gfx::Rect(0, 0, image_size.width(), image_size.height()));

  root_view->AddChildView(this);

  SetVisible(false);
}

AIChatCursorOverlay::~AIChatCursorOverlay() = default;

void AIChatCursorOverlay::MoveCursorTo(int x, int y) {
  gfx::Rect start_bounds = bounds();

  // The new position, keeping the same width/height as the current bounds.
  gfx::Rect target_bounds(x, y, start_bounds.width(), start_bounds.height());

  // Scope the animation settings to ensure we only animate once (and can customize easing, etc.).
  ui::ScopedLayerAnimationSettings settings(layer()->GetAnimator());
  settings.SetTransitionDuration(base::Milliseconds(1000));
  settings.SetTweenType(gfx::Tween::LINEAR);
  // Possible tween types: EASE_IN, EASE_OUT, FAST_OUT_SLOW_IN, etc.

  // Trigger the animation by setting new bounds. Chromium will animate from
  // the old layer bounds to these new layer bounds.
  SetBoundsRect(target_bounds);


  // SetPosition(gfx::Point(x, y));
  LOG(ERROR) << "moving cursor to " << x << ", " << y;
  // SetBoundsRect(gfx::Rect(x, y, 500, 500));
}

void AIChatCursorOverlay::ShowCursor() {
  SetVisible(true);
}

void AIChatCursorOverlay::HideCursor() {
  SetVisible(false);
}

void AIChatCursorOverlay::OnBoundsChanged(const gfx::Rect& previous_bounds) {
  views::View::OnBoundsChanged(previous_bounds);
}

BEGIN_METADATA(AIChatCursorOverlay)
END_METADATA
