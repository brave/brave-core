// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/brave_help_bubble/brave_help_bubble_host_view.h"

#include "cc/paint/paint_shader.h"
#include "extensions/common/constants.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkPoint.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/ui_base_types.h"
#include "ui/compositor/layer.h"
#include "ui/gfx/animation/animation.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/transform_util.h"
#include "ui/views/animation/animation_builder.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/bubble/bubble_frame_view.h"
#include "ui/views/view_class_properties.h"
#include "ui/views/view_utils.h"

namespace {
constexpr int kWidth = 60;
constexpr int kHeight = 60;

// The points here are defined as start and end points of the gradient,
// associated with an entire rect
constexpr SkPoint kPts[] = {{0, 0}, {kWidth, kHeight}};

// Colors are ported from Figma, but the order is intentionally flipped for
// proper gradient interpolation. Figma has the order as 1, 2, 3, here it's
// represented as 3, 2, 1
constexpr SkColor4f kColors[] = {{0.65f, 0.54f, 1, 1},
                                 {1, 0.09f, 0.57f, 1},
                                 {0.98f, 0.44f, 0.31f, 1}};

// These are positions for each element in kColors
constexpr SkScalar kPositions[] = {0.0, 0.43, 0.93};

sk_sp<cc::PaintShader> kBraveGradient =
    cc::PaintShader::MakeLinearGradient(kPts,
                                        kColors,
                                        kPositions,
                                        std::size(kColors),
                                        SkTileMode::kClamp);

void SchedulePulsingAnimation(ui::Layer* layer) {
  DCHECK(layer);

  constexpr base::TimeDelta kPulsingDuration = base::Milliseconds(1000);

  const gfx::Rect local_bounds(layer->bounds().size());
  views::AnimationBuilder()
      .SetPreemptionStrategy(
          ui::LayerAnimator::IMMEDIATELY_ANIMATE_TO_NEW_TARGET)
      .Repeatedly()
      .SetDuration(kPulsingDuration)
      .SetTransform(layer,
                    gfx::GetScaleTransform(local_bounds.CenterPoint(), 0.7f),
                    gfx::Tween::EASE_IN)
      .At(kPulsingDuration)
      .SetDuration(kPulsingDuration)
      .SetTransform(layer,
                    gfx::GetScaleTransform(local_bounds.CenterPoint(), 1.0f),
                    gfx::Tween::EASE_OUT);
}
}  // namespace

BraveHelpBubbleHostView::BraveHelpBubbleHostView() {
  // Disable event handling to interact with the underlying element.
  SetCanProcessEventsWithinSubtree(false);
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);
  SetSize({kWidth, kHeight});
}

BraveHelpBubbleHostView::~BraveHelpBubbleHostView() = default;

bool BraveHelpBubbleHostView::Show() {
  if (help_bubble_) {
    return false;
  }

  CHECK(tracked_element_ && !text_.empty());

  auto* brave_help_bubble_delegate_view =
      new BraveHelpBubbleDelegateView(this, text_);
  help_bubble_ = views::BubbleDialogDelegateView::CreateBubble(
      brave_help_bubble_delegate_view);
  auto* frame_view = brave_help_bubble_delegate_view->GetBubbleFrameView();
  frame_view->SetDisplayVisibleArrow(true);
  bubble_widget_observation_.Observe(help_bubble_);

  // Observes tracked element and host widget(browser frame) to know this host
  // view's position update timing.
  tracked_view_observation_.Observe(tracked_element_);
  host_widget_observation_.Observe(GetWidget());

  // To locate help bubble at more higher than other normal widget.
  help_bubble_->SetZOrderLevel(ui::ZOrderLevel::kFloatingUIElement);

  ui::ElementIdentifier id =
      tracked_element_->GetProperty(views::kElementIdentifierKey);
  CHECK(id);
  // Listen activation state to to hide bubble.(ex, hide bubble when button
  // clicked)
  activated_subscription_ =
      ui::ElementTracker::GetElementTracker()->AddElementActivatedCallback(
          id, views::ElementTrackerViews::GetContextForView(tracked_element_),
          base::BindRepeating(
              &BraveHelpBubbleHostView::OnTrackedElementActivated,
              weak_factory_.GetWeakPtr()));

  // With this inactive launching, bubble will be hidden after activated.
  help_bubble_->ShowInactive();

  UpdatePosition();
  SetVisible(true);

  if (!gfx::Animation::ShouldRenderRichAnimation()) {
    SchedulePulsingAnimation(layer());
  }

  return true;
}

void BraveHelpBubbleHostView::Hide() {
  if (!help_bubble_) {
    return;
  }

  // Closing bubble will make this host view hidden.
  help_bubble_->CloseWithReason(views::Widget::ClosedReason::kLostFocus);
}

void BraveHelpBubbleHostView::UpdatePosition() {
  CHECK(tracked_element_);
  auto tracked_element_local_center =
      tracked_element_->GetLocalBounds().CenterPoint();
  views::View::ConvertPointToScreen(tracked_element_,
                                    &tracked_element_local_center);
  auto host_view_origin = views::View::ConvertPointFromScreen(
      this->parent(), tracked_element_local_center);
  host_view_origin.Offset(-kWidth / 2, -kHeight / 2);
  SetPosition(host_view_origin);
}

void BraveHelpBubbleHostView::OnPaint(gfx::Canvas* canvas) {
  cc::PaintFlags flags;
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setShader(kBraveGradient);
  flags.setStrokeWidth(2.f);
  canvas->DrawCircle(GetContentsBounds().CenterPoint(), 27.f, flags);
  flags.setStrokeWidth(6.f);
  canvas->DrawCircle(GetContentsBounds().CenterPoint(), 20.f, flags);
}

void BraveHelpBubbleHostView::OnViewBoundsChanged(views::View* observed_view) {
  // Update host view position when |tracked_element_| bounds is changed.
  CHECK(tracked_element_ == observed_view);
  UpdatePosition();
}

void BraveHelpBubbleHostView::OnViewIsDeleting(views::View* observed_view) {
  CHECK(tracked_element_ == observed_view);
  tracked_element_ = nullptr;
  tracked_view_observation_.Reset();
}

void BraveHelpBubbleHostView::OnViewVisibilityChanged(
    views::View* observed_view,
    views::View* starting_view) {
  // Close help bubble when |tracked_element_| gets hidden.
  // It could be in-visible when its ancestor is hidden.
  if (!observed_view->GetVisible() ||
      (starting_view && !starting_view->GetVisible())) {
    Hide();
  }
}

void BraveHelpBubbleHostView::OnWidgetBoundsChanged(views::Widget* widget,
                                                    const gfx::Rect&) {
  // Only update host view position when host widget(browser frame)'s bounds is
  // changed.
  if (help_bubble_ == widget) {
    return;
  }

  UpdatePosition();
}

void BraveHelpBubbleHostView::OnWidgetDestroying(views::Widget* widget) {
  if (help_bubble_ == widget) {
    // Hide this host view when bubble is closed.
    help_bubble_ = nullptr;
    bubble_widget_observation_.Reset();
    tracked_view_observation_.Reset();
    text_ = std::string();
    activated_subscription_ = {};
    tracked_element_ = nullptr;

    SetVisible(false);
  }

  host_widget_observation_.Reset();
}

void BraveHelpBubbleHostView::OnTrackedElementActivated(
    ui::TrackedElement* element) {
  Hide();
}

BEGIN_METADATA(BraveHelpBubbleHostView, View)
END_METADATA
