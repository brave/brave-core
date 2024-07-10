/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/location_bar/onion_location_view.h"

#include <memory>
#include <utility>

#include "base/memory/raw_ptr.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/tor/tor_profile_manager.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/components/tor/onion_location_tab_helper.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/views/toolbar/toolbar_ink_drop_util.h"
#include "components/grit/brave_components_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/models/image_model.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/geometry/skia_conversions.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/text_constants.h"
#include "ui/views/animation/ink_drop_impl.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/highlight_path_generator.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/painter.h"
#include "ui/views/view.h"

namespace {

constexpr SkColor kOpenInTorBg = SkColorSetRGB(0x8c, 0x30, 0xbb);
constexpr SkColor kIconColor = SK_ColorWHITE;
constexpr SkColor kTextColor = SK_ColorWHITE;
constexpr int kFontHeight = 18;
constexpr int kIconSize = 18;
constexpr int kIconLabelSpacing = 4;
constexpr int kCornerRadius = 8;
constexpr auto kButtonInsets = gfx::Insets::TLBR(5, 4, 5, 4);
constexpr auto kButtonInsetsTor = gfx::Insets::TLBR(5, 4, 5, 8);
constexpr auto kViewInsets = gfx::Insets::VH(1, 3);

// Sets the focus and ink drop highlight path to match the background
// along with it's corner radius.
class HighlightPathGenerator : public views::HighlightPathGenerator {
 public:
  HighlightPathGenerator() = default;
  HighlightPathGenerator(const HighlightPathGenerator&) = delete;
  HighlightPathGenerator& operator=(const HighlightPathGenerator&) = delete;

  // views::HighlightPathGenerator:
  SkPath GetHighlightPath(const views::View* view) override {
    const gfx::Rect highlight_bounds = view->GetLocalBounds();
    const SkRect rect = RectToSkRect(highlight_bounds);
    return SkPath().addRoundRect(rect, kCornerRadius, kCornerRadius);
  }
};

class OnionLocationButtonView : public views::LabelButton {
 public:
  explicit OnionLocationButtonView(Profile* profile)
      : LabelButton(base::BindRepeating(&OnionLocationButtonView::ButtonPressed,
                                        base::Unretained(this))),
        profile_(profile) {
    if (profile_->IsTor()) {
      SetText(brave_l10n::GetLocalizedResourceUTF16String(
          IDS_LOCATION_BAR_ONION_AVAILABLE));
    }

    // Set text style specifics
    SetEnabledTextColors(kTextColor);
    SetTextColor(views::Button::STATE_DISABLED, kTextColor);
    label()->SetFontList(
        label()->font_list().DeriveWithHeightUpperBound(kFontHeight));

    // Render vector icon
    SetImageModel(views::Button::STATE_NORMAL,
                  ui::ImageModel::FromVectorIcon(kLeoProductTorIcon, kIconColor,
                                                 kIconSize));

    // Show icon on the left
    SetHorizontalAlignment(gfx::ALIGN_LEFT);
    SetImageLabelSpacing(kIconLabelSpacing);

    auto* ink_drop = views::InkDrop::Get(this);
    ink_drop->SetMode(views::InkDropHost::InkDropMode::ON);
    SetBorder(views::CreateEmptyBorder(profile_->IsTor() ? kButtonInsetsTor
                                                         : kButtonInsets));
    SetHasInkDropActionOnClick(true);
    ink_drop->SetVisibleOpacity(kToolbarInkDropVisibleOpacity);
    UpdateBorder();
    // Ensure focus ring follows border
    views::HighlightPathGenerator::Install(
        this, std::make_unique<HighlightPathGenerator>());
  }

  OnionLocationButtonView(const OnionLocationButtonView&) = delete;
  OnionLocationButtonView& operator=(const OnionLocationButtonView&) = delete;

  ~OnionLocationButtonView() override = default;

  void SetOnionLocation(const GURL& location) {
    onion_location_ = location;
    SetTooltipText(l10n_util::GetStringFUTF16(
        profile_->IsTor() ? IDS_LOCATION_BAR_ONION_AVAILABLE_TOOLTIP_TEXT
                          : IDS_LOCATION_BAR_OPEN_IN_TOR_TOOLTIP_TEXT,
        base::UTF8ToUTF16(onion_location_.spec())));
  }

 private:
  // views::View
  void Layout(PassKey) override {
    LayoutSuperclass<views::LabelButton>(this);
    UpdateBorder();
  }

  void UpdateBorder() {
    SetBackground(
        views::CreateRoundedRectBackground(kOpenInTorBg, kCornerRadius));
  }

  void ButtonPressed() {
    TorProfileManager::SwitchToTorProfile(profile_, onion_location_);
  }

  GURL onion_location_;
  raw_ptr<Profile> profile_ = nullptr;
};

}  // namespace

OnionLocationView::OnionLocationView(Profile* profile) {
  SetBorder(views::CreateEmptyBorder(kViewInsets));
  SetVisible(false);
  // automatic layout
  auto vertical_container_layout = std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal);
  vertical_container_layout->set_main_axis_alignment(
      views::BoxLayout::MainAxisAlignment::kCenter);
  vertical_container_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  SetLayoutManager(std::move(vertical_container_layout));

  button_ = new OnionLocationButtonView(profile);
  AddChildView(button_.get());
}

OnionLocationView::~OnionLocationView() = default;

void OnionLocationView::Update(content::WebContents* web_contents,
                               bool show_page_actions) {
  if (!web_contents) {
    return;
  }
  tor::OnionLocationTabHelper* helper =
      tor::OnionLocationTabHelper::FromWebContents(web_contents);
  const bool show_icon =
      helper && helper->should_show_icon() && show_page_actions;

  SetVisible(show_icon);
  if (show_icon) {
    reinterpret_cast<OnionLocationButtonView*>(button_.get())
        ->SetOnionLocation(helper->onion_location());
  }
}

BEGIN_METADATA(OnionLocationView)
END_METADATA
