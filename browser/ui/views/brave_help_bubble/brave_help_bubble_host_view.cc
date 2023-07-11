// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/brave_help_bubble/brave_help_bubble_host_view.h"

#include "cc/paint/paint_shader.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkPoint.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/compositor/layer.h"
#include "ui/gfx/animation/animation.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/transform_util.h"
#include "ui/views/animation/animation_builder.h"
#include "ui/views/background.h"
#include "ui/views/bubble/bubble_border.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/view_class_properties.h"

namespace {
constexpr int kWidth = 60;
constexpr int kHeight = 60;
constexpr base::TimeDelta kPulsingDuration = base::Milliseconds(1000);
constexpr SkPoint kPts[] = {{0, 0}, {60, 60}};
constexpr SkColor4f kColors[] = {{0.66, 0.10, 0.47, 1.f},
                                 {0.22, 0.17, 0.81, 1.f}};
constexpr SkScalar kPositions[] = {0.0, 0.65, 1.0};
sk_sp<cc::PaintShader> kBraveGradient =
    cc::PaintShader::MakeLinearGradient(kPts,
                                        kColors,
                                        kPositions,
                                        std::size(kPts),
                                        SkTileMode::kClamp);

void SchedulePulsingAnimation(ui::Layer* layer) {
  DCHECK(layer);

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

BraveHelpBubbleHostView::BraveHelpBubbleHostView(View* tracked_element)
    : tracked_element_(tracked_element) {
  // Making the layer of this view transparent for mouse clicks to interact with
  // the underlying tracked element
  SetEnabled(false);
}

BraveHelpBubbleHostView::~BraveHelpBubbleHostView() = default;

// static
base::WeakPtr<BraveHelpBubbleHostView> BraveHelpBubbleHostView::Create(
    View* tracked_element,
    const std::string text) {
  auto* bubble = new BraveHelpBubbleHostView(tracked_element);
  bubble->text_ = text;
  return bubble->weak_factory_.GetWeakPtr();
}

void BraveHelpBubbleHostView::Show() {
  if (!brave_help_bubble_delegate_view_) {
    brave_help_bubble_delegate_view_ =
        new BraveHelpBubbleDelegateView(this, text_);
    bubble_help_delegate_observation_.Observe(brave_help_bubble_delegate_view_);
  }

  if (brave_help_bubble_delegate_view_) {
    brave_help_bubble_delegate_view_->Show();
  }

  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);
  SetSize({kWidth, kHeight});
  if (!gfx::Animation::PrefersReducedMotion()) {
    SchedulePulsingAnimation(layer());
  }
  SetVisible(true);
}

void BraveHelpBubbleHostView::Hide() {
  if (brave_help_bubble_delegate_view_) {
    brave_help_bubble_delegate_view_->Hide();
  }

  SetVisible(false);
}

void BraveHelpBubbleHostView::InitElementTrackers() {
  ui::ElementIdentifier id =
      tracked_element_->GetProperty(views::kElementIdentifierKey);

  if (!id) {
    id = ui::ElementTracker::kTemporaryIdentifier;
    tracked_element_->SetProperty(views::kElementIdentifierKey, id);
  }

  context_ = views::ElementTrackerViews::GetContextForView(tracked_element_);
  CHECK(context_);

  shown_subscription_ =
      ui::ElementTracker::GetElementTracker()->AddElementShownCallback(
          id, context_,
          base::BindRepeating(&BraveHelpBubbleHostView::OnTrackedElementShown,
                              weak_factory_.GetWeakPtr()));
  hidden_subscription_ =
      ui::ElementTracker::GetElementTracker()->AddElementHiddenCallback(
          id, context_,
          base::BindRepeating(&BraveHelpBubbleHostView::OnTrackedElementHidden,
                              weak_factory_.GetWeakPtr()));
}

void BraveHelpBubbleHostView::UpdatePosition() {
  auto browser_root_view_origin =
      GetWidget()->GetRootView()->GetBoundsInScreen().origin();
  auto tracked_element_origin =
      tracked_element_->GetBoundsInScreen().CenterPoint();

  auto circle_center = gfx::Point(kWidth / 2, kHeight / 2);

  // Calculate the final origin point by taking into account the Browser
  // window's position and frame's top padding
  tracked_element_origin.set_x(tracked_element_origin.x() -
                               browser_root_view_origin.x() -
                               circle_center.x() + kBraveActionLeftMarginExtra);
  tracked_element_origin.set_y(tracked_element_origin.y() -
                               browser_root_view_origin.y() -
                               circle_center.y());

  SetPosition({tracked_element_origin.x(), tracked_element_origin.y()});
}

void BraveHelpBubbleHostView::StartObservingAndShow() {
  // Observing changes to the parent element's bounds,
  // as they are the only ones that are expected to change
  if (!view_observation_.IsObserving()) {
    view_observation_.Observe(tracked_element_->parent());
  }

  UpdatePosition();
  Show();
}

void BraveHelpBubbleHostView::AddedToWidget() {
  InitElementTrackers();

  if (tracked_element_ && tracked_element_->GetVisible()) {
    StartObservingAndShow();
  }
}

void BraveHelpBubbleHostView::OnPaint(gfx::Canvas* canvas) {
  cc::PaintFlags flags;
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setShader(kBraveGradient);
  flags.setStrokeWidth(2.f);
  canvas->DrawCircle(GetContentsBounds().CenterPoint(), 27.f, flags);
  flags.setStrokeWidth(5.f);
  canvas->DrawCircle(GetContentsBounds().CenterPoint(), 20.f, flags);
}

void BraveHelpBubbleHostView::OnBubbleDestroying(views::Widget* widget) {
  // In the destruction, we don't have to remove this from parent. Otherwise,
  // DCHECK will fail as the ancestor view is iterating its children to destroy
  // all descendant.
  if (GetWidget()->IsClosed()) {
    return;
  }

  // There's a possibility that someone is iterating over the parent's children,
  // and we are removing the child view early. This can cause a DCHECK in
  // view.cc, so we schedule removal of the child view via TaskRunner to avoid
  // this issue.
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(
                     [](base::WeakPtr<views::View> view) {
                       if (!view) {
                         return;
                       }
                       DCHECK(view->parent());
                       view->parent()->RemoveChildViewT(view.get());
                     },
                     weak_factory_.GetWeakPtr()));

  brave_help_bubble_delegate_view_ = nullptr;
}

void BraveHelpBubbleHostView::OnViewBoundsChanged(views::View* observed_view) {
  UpdatePosition();
}

void BraveHelpBubbleHostView::OnTrackedElementShown(
    ui::TrackedElement* element) {
  if (auto* el = element->AsA<views::TrackedElementViews>();
      !el || el->view() != tracked_element_) {
    return;
  }

  StartObservingAndShow();
}

void BraveHelpBubbleHostView::OnTrackedElementHidden(
    ui::TrackedElement* element) {
  Hide();
}

BEGIN_METADATA(BraveHelpBubbleHostView, View)
END_METADATA
