/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_ads/padded_image_button.h"

#include <algorithm>
#include <utility>

#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/animation/ink_drop.h"
#include "ui/views/background.h"
#include "ui/views/border.h"

namespace brave_ads {

namespace {

constexpr SkColor kBackgroundColor = SK_ColorTRANSPARENT;

constexpr auto kBorderInset = gfx::Insets::TLBR(4, 4, 0, 4);

constexpr float kVisibleOpacity = 0.12f;

}  // namespace

PaddedImageButton::PaddedImageButton(PressedCallback callback)
    : views::ImageButton(std::move(callback)) {
  auto* ink_drop = views::InkDrop::Get(this);
  views::InkDrop::UseInkDropForSquareRipple(ink_drop,
                                            /*highlight_on_hover=*/false,
                                            /*highlight_on_focus=*/false);

  SetBackground(views::CreateSolidBackground(kBackgroundColor));

  SetBorder(views::CreateEmptyBorder(kBorderInset));

  SetAnimateOnStateChange(false);

  ink_drop->SetMode(views::InkDropHost::InkDropMode::ON);
  ink_drop->SetVisibleOpacity(kVisibleOpacity);
  SetHasInkDropActionOnClick(true);
}

void PaddedImageButton::AdjustBorderInsetToFitHeight(int height) {
  views::Border* border = GetBorder();
  CHECK(border);

  const gfx::ImageSkia& image = GetImage(views::Button::STATE_NORMAL);

  gfx::Insets insets = border->GetInsets();
  const int inset = std::max(0, height - insets.height() - image.height());

  insets += gfx::Insets::TLBR(0, 0, inset, 0);
  SetBorder(views::CreateEmptyBorder(insets));
}

void PaddedImageButton::OnThemeChanged() {
  ImageButton::OnThemeChanged();

  const SkColor background_color =
      GetColorProvider()->GetColor(ui::kColorWindowBackground);
  views::InkDrop::Get(this)->SetBaseColor(
      color_utils::GetColorWithMaxContrast(background_color));
}

BEGIN_METADATA(PaddedImageButton)
END_METADATA

}  // namespace brave_ads
