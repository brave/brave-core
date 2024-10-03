/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/playlist/selectable_list_view.h"

#include "base/strings/utf_string_conversions.h"
#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/views/playlist/thumbnail_view.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/background.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"

SelectableView::SelectableView(const std::string& id,
                               const std::string& name,
                               const gfx::Image& image,
                               OnPressedCallback on_pressed)
    : Button(base::BindRepeating(&SelectableView::OnPressed,
                                 base::Unretained(this))
                 .Then(on_pressed)),
      id_(id),
      name_(name),
      image_(image) {
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal, gfx::Insets(8),
      /*between_child_spacing=*/16));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  SetPreferredSize(gfx::Size(288, 64));

  constexpr gfx::Size kThumbnailSize(64, 48);
  thumbnail_view_ = AddChildView(std::make_unique<ThumbnailView>(image_));
  thumbnail_view_->SetPreferredSize(kThumbnailSize);

  auto* title =
      AddChildView(std::make_unique<views::Label>(base::UTF8ToUTF16(name_)));
  title->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  layout->SetFlexForView(title, 1);

  selected_icon_ = AddChildView(std::make_unique<views::ImageView>());
  selected_icon_->SetImage(ui::ImageModel::FromVectorIcon(
      kLeoCheckCircleOutlineIcon, kColorBravePlaylistCheckedIcon, 20));
  selected_icon_->SetVisible(selected_);
}

SelectableView::~SelectableView() = default;

void SelectableView::OnThemeChanged() {
  views::Button::OnThemeChanged();
  UpdateBackground();
}

SelectableView& SelectableView::OnPressed(const ui::Event& event) {
  SetSelected(!selected_);
  return *this;
}

void SelectableView::SetSelected(bool selected) {
  if (selected_ == selected) {
    return;
  }

  selected_ = selected;
  selected_icon_->SetVisible(selected_);

  UpdateBackground();
}

base::OnceCallback<void(const gfx::Image&)>
SelectableView::GetThumbnailSetter() {
  return thumbnail_view_->GetThumbnailSetter();
}

void SelectableView::UpdateBackground() {
  auto* cp = GetColorProvider();
  if (!cp) {
    // Not attached to widget yet.
    return;
  }

  if (selected_) {
    SetBackground(views::CreateSolidBackground(
        cp->GetColor(kColorBravePlaylistSelectedBackground)));
  } else {
    SetBackground(nullptr);
  }
}

BEGIN_METADATA(SelectableView)
END_METADATA
