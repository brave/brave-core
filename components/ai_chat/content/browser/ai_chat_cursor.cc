// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/content/browser/ai_chat_cursor.h"

#include "base/check.h"
// TODO(petemill): Obviously this isn't allowed here, this class will be moved
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/scoped_layer_animation_settings.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/background.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

namespace {
  constexpr size_t kIconSize = 40;
}

AIChatCursorOverlay::AIChatCursorOverlay(content::WebContents* web_contents) {
  DCHECK(web_contents);

  // TODO: This should be done in the browser views layer, with a tab strip
  // observer to obey the correct layering, to hide and show when active
  // contents changes, and to move to a new window when the webcontents moves.

  // Experiment without layer violation (gets the whole browser window not the content view)
  // auto* root_view = views::Widget::GetWidgetForNativeView(web_contents->GetNativeView())->GetContentsView();

  auto* root_view = chrome::FindBrowserWithTab(web_contents)->GetBrowserView().contents_container();
  DCHECK(root_view);

  cursor_image_ = AddChildView(std::make_unique<views::ImageView>());
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);

  // Experiment Vector icon (doesn't work well due to opacity)
  // auto icon = gfx::CreateVectorIcon(kAiChatCursorIcon, SK_ColorTRANSPARENT);
  // cursor_image_->SetImage(icon);

  // PNG resource
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  gfx::ImageSkia* image = rb.GetImageSkiaNamed(IDR_AI_CHAT_UI_CURSOR);
  if (!image) {
    DLOG(ERROR) << "Failed to load cursor image resource!";
    return;
  }

  cursor_image_->SetImage(*image);
  cursor_image_->SetImageSize(gfx::Size(kIconSize, kIconSize));

  root_view->AddChildView(this);

  cursor_image_->SetBounds(0, 0, kIconSize, kIconSize);
  SetBoundsRect(gfx::Rect(0, 0, kIconSize, kIconSize));

  SetVisible(true);
}

AIChatCursorOverlay::~AIChatCursorOverlay() {
  if (parent())
    parent()->RemoveChildView(this);
}

void AIChatCursorOverlay::MoveCursorTo(int x, int y) {
  gfx::Rect start_bounds = bounds();

  // The new position, keeping the same width/height as the current bounds.
  gfx::Rect target_bounds(x, y, start_bounds.width(), start_bounds.height());

  // Scope the animation settings to ensure we only animate once (and can customize easing, etc.).
  ui::ScopedLayerAnimationSettings settings(layer()->GetAnimator());
  settings.SetTransitionDuration(base::Milliseconds(1000));
  settings.SetTweenType(gfx::Tween::EASE_IN_2);
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
  // TODO(petemill): Fade out and notify caller so the class can be deleted
  // and re-created on whichever browser the tab is in next time the cursor
  // is needed.
  SetVisible(false);
}

BEGIN_METADATA(AIChatCursorOverlay)
END_METADATA
