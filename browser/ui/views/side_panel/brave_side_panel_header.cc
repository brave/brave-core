/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/side_panel/brave_side_panel_header.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "brave/ui/color/nala/nala_color_id.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/separator.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/flex_layout_types.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/view_class_properties.h"

namespace {

constexpr int kHeaderInteriorMargin = 16;
constexpr int kHeaderButtonSize = 20;
constexpr int kSeparatorHorizontalSpacing = 12;

}  // namespace

BraveSidePanelHeader::BraveSidePanelHeader(std::unique_ptr<Delegate> delegate)
    : delegate_(std::move(delegate)) {
  CHECK(delegate_);

  UpdateHeader();
  SetLayoutManager(std::make_unique<views::FlexLayout>())
      ->SetOrientation(views::LayoutOrientation::kHorizontal)
      .SetInteriorMargin(gfx::Insets(kHeaderInteriorMargin))
      .SetMainAxisAlignment(views::LayoutAlignment::kStart)
      .SetCrossAxisAlignment(views::LayoutAlignment::kCenter);

  AddChildView(delegate_->CreatePanelTitle());

  AddChildView(std::make_unique<views::View>())
      ->SetProperty(
          views::kFlexBehaviorKey,
          views::FlexSpecification(views::MinimumFlexSizeRule::kScaleToZero,
                                   views::MaximumFlexSizeRule::kUnbounded)
              .WithOrder(2));

  if (auto launch_button = delegate_->CreateLaunchButton()) {
    AddChildView(std::move(launch_button));
    auto* separator = AddChildView(std::make_unique<views::Separator>());
    separator->SetColorId(nala::kColorDividerSubtle);
    separator->SetPreferredLength(kHeaderButtonSize);
    separator->SetProperty(views::kMarginsKey,
                           gfx::Insets::VH(0, kSeparatorHorizontalSpacing));
  }

  AddChildView(delegate_->CreateCloseButton());

  // Safe: `delegate_` is owned by this view and destroyed before `this`, so
  // the callback cannot outlive `this`.
  delegate_->SetUpdateHeaderCallback(base::BindRepeating(
      &BraveSidePanelHeader::UpdateHeader, base::Unretained(this)));
}

BraveSidePanelHeader::~BraveSidePanelHeader() = default;

void BraveSidePanelHeader::UpdateHeader() {
  SetBackground(views::CreateRoundedRectBackground(
      nala::kColorPageBackground, delegate_->GetTopRadius(), 0, 0));
}

void BraveSidePanelHeader::Layout(PassKey) {
  LayoutSuperclass<views::View>(this);

  // Need to set bounds as parent view(SidePanel) uses FillLayout.
  const gfx::Rect contents_bounds = parent()->GetContentsBounds();
  SetBoundsRect(gfx::Rect(
      contents_bounds.x(), contents_bounds.y() - GetPreferredSize().height(),
      contents_bounds.width(), GetPreferredSize().height()));
}

BEGIN_METADATA(BraveSidePanelHeader)
END_METADATA
