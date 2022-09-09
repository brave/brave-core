// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/brave_help_bubble/brave_help_bubble_view.h"

#include "cc/paint/paint_shader.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkPoint.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/compositor/layer.h"
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
constexpr SkColor kBlockColor = SkColorSetRGB(52, 172, 224);
constexpr SkPoint kPts[] = {{0, 0}, {60, 60}};
constexpr SkColor4f kColors[] = {{0.66, 0.10, 0.47, 1.f},
                                 {0.22, 0.17, 0.81, 1.f}};
constexpr SkScalar kPositions[] = {0.0, 0.65, 1.0};
sk_sp<cc::PaintShader> kBraveGradient =
    cc::PaintShader::MakeLinearGradient(kPts,
                                        kColors,
                                        kPositions,
                                        3,
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

BraveHelpBubbleView::BraveHelpBubbleView(View* tracked_element)
    : tracked_element_(tracked_element) {
  SetEnabled(false);
}

// static
base::WeakPtr<BraveHelpBubbleView> BraveHelpBubbleView::Create(
    View* tracked_element,
    const std::u16string text) {
  auto* bubble = new BraveHelpBubbleView(tracked_element);
  bubble->text_ = text;
  return bubble->weak_factory_.GetWeakPtr();
}

BraveHelpBubbleView::~BraveHelpBubbleView() {
  if (brave_help_bubble_delegate_) {
    brave_help_bubble_delegate_->RemoveObserver(this);
  }

  scoped_observation_.Reset();
}

void BraveHelpBubbleView::InitElementTrackers() {
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
          base::BindRepeating(&BraveHelpBubbleView::OnTrackedElementShown,
                              base::Unretained(this)));
  hidden_subscription_ =
      ui::ElementTracker::GetElementTracker()->AddElementHiddenCallback(
          id, context_,
          base::BindRepeating(&BraveHelpBubbleView::OnTrackedElementHidden,
                              base::Unretained(this)));
}

void BraveHelpBubbleView::AddedToWidget() {
  InitElementTrackers();

  if (tracked_element_ && tracked_element_->GetVisible()) {
    UpdatePosition();
    Show();
  }
}

void BraveHelpBubbleView::UpdatePosition() {
  auto tracked_element_origin =
      tracked_element_->GetBoundsInScreen().CenterPoint();
  tracked_element_origin.set_x(tracked_element_origin.x() - (kWidth / 2) - 1.f);
  tracked_element_origin.set_y(tracked_element_origin.y() - kHeight - 15);

  SetPosition({tracked_element_origin.x(), tracked_element_origin.y()});
}

void BraveHelpBubbleView::Show() {
  if (!brave_help_bubble_delegate_) {
    brave_help_bubble_delegate_ = new BraveHelpBubbleDelegate(this, text_);
    brave_help_bubble_delegate_->AddObserver(this);
  }

  if (brave_help_bubble_delegate_) {
    brave_help_bubble_delegate_->Show();
  }

  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);
  SetSize({kWidth, kHeight});
  SchedulePulsingAnimation(layer());
  SetVisible(true);
}

void BraveHelpBubbleView::Hide() {
  if (brave_help_bubble_delegate_) {
    brave_help_bubble_delegate_->Hide();
  }

  SetVisible(false);
}

void BraveHelpBubbleView::OnBubbleClosing(Widget* widget) {
  // In the destruction, we don't have to remove this from parent. Otherwise,
  // DCHECK will fail as the ancestor view is iterating its children to destroy
  // all descendant.
  if (GetWidget()->IsClosed())
    return;

  // There's a possibility that someone is iterating over the parent's children,
  // and we are removing the child view early. This can cause a DCHECK in
  // view.cc, so we schedule removal of the child view via TaskRunner to avoid
  // this issue.
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](base::WeakPtr<views::View> view, View* this_element) {
            if (!view)
              return;
            if (!view->parent() || !view->parent()->Contains(view.get()))
              return;
            view->parent()->RemoveChildView(this_element);
          },
          weak_factory_.GetWeakPtr(), this));

  brave_help_bubble_delegate_ = nullptr;
  base::SequencedTaskRunner::GetCurrentDefault()->DeleteSoon(FROM_HERE, this);
}

void BraveHelpBubbleView::OnPaint(gfx::Canvas* canvas) {
  cc::PaintFlags flags;
  flags.setColor(kBlockColor);
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setShader(kBraveGradient);
  flags.setStrokeWidth(2.f);
  canvas->DrawCircle(GetContentsBounds().CenterPoint(), 27.f, flags);
  flags.setStrokeWidth(5.f);
  canvas->DrawCircle(GetContentsBounds().CenterPoint(), 20.f, flags);
}

void BraveHelpBubbleView::OnTrackedElementShown(ui::TrackedElement* element) {
  auto* el = element->AsA<views::TrackedElementViews>();

  if (!el || el->view() != tracked_element_)
    return;

  // Observing changes to the parent element's bounds,
  // as they are the only ones that are expected to change
  scoped_observation_.Observe(tracked_element_->parent());
  UpdatePosition();
  Show();
}

void BraveHelpBubbleView::OnTrackedElementHidden(ui::TrackedElement* element) {
  Hide();
}

void BraveHelpBubbleView::OnViewBoundsChanged(views::View* observed_view) {
  UpdatePosition();
}

BEGIN_METADATA(BraveHelpBubbleView, View)
END_METADATA
