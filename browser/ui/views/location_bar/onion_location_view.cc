/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/location_bar/onion_location_view.h"

#include <memory>
#include <utility>

#include "base/memory/raw_ptr.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/tor/tor_profile_manager.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/components/tor/onion_location_tab_helper.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/views/toolbar/toolbar_ink_drop_util.h"
#include "components/grit/brave_components_resources.h"
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

constexpr SkColor kOpenInTorBg = SkColorSetRGB(0x6a, 0x37, 0x85);
constexpr SkColor kIconColor = SkColorSetRGB(0xf0, 0xf2, 0xff);
constexpr SkColor kTextColor = SK_ColorWHITE;
constexpr int kIconSize = 12;

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
    const int corner_radius = view->height() / 2;
    return SkPath().addRoundRect(rect, corner_radius, corner_radius);
  }
};

class OnionLocationButtonView : public views::LabelButton {
 public:
  explicit OnionLocationButtonView(Profile* profile)
      : LabelButton(base::BindRepeating(&OnionLocationButtonView::ButtonPressed,
                                        base::Unretained(this)),
                    brave_l10n::GetLocalizedResourceUTF16String(
                        IDS_LOCATION_BAR_OPEN_IN_TOR)),
        profile_(profile) {
    SetTooltipText(brave_l10n::GetLocalizedResourceUTF16String(
        IDS_LOCATION_BAR_OPEN_IN_TOR_TOOLTIP_TEXT));
    if (profile->IsTor()) {
      SetText(brave_l10n::GetLocalizedResourceUTF16String(
          IDS_LOCATION_BAR_ONION_AVAILABLE));
      SetTooltipText(brave_l10n::GetLocalizedResourceUTF16String(
          IDS_LOCATION_BAR_ONION_AVAILABLE_TOOLTIP_TEXT));
    }
    // Render vector icon
    const gfx::ImageSkia image =
        gfx::CreateVectorIcon(kOpenInTorIcon, kIconSize, kIconColor);
    SetImageModel(views::Button::STATE_NORMAL,
                  ui::ImageModel::FromImageSkia(image));
    // Set style specifics
    SetEnabledTextColors(kTextColor);
    SetHorizontalAlignment(gfx::ALIGN_RIGHT);
    SetImageLabelSpacing(6);

    auto* ink_drop = views::InkDrop::Get(this);
    ink_drop->SetMode(views::InkDropHost::InkDropMode::ON);
    SetBorder(views::CreateEmptyBorder(
        GetLayoutInsets(LOCATION_BAR_ICON_INTERIOR_PADDING)));
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

  void SetOnionLocation(GURL location) { onion_location_ = location; }

 private:
  // views::View
  void Layout() override {
    views::LabelButton::Layout();
    UpdateBorder();
  }

  void UpdateBorder() {
    SetBackground(
        views::CreateRoundedRectBackground(kOpenInTorBg, height() / 2));
  }

  void ButtonPressed() {
    if (Browser* tor_browser =
            TorProfileManager::SwitchToTorProfile(profile_)) {
      content::OpenURLParams open_tor(onion_location_, content::Referrer(),
                                      WindowOpenDisposition::SWITCH_TO_TAB,
                                      ui::PAGE_TRANSITION_TYPED, false);
      tor_browser->OpenURL(open_tor);
    }
  }

  GURL onion_location_;
  raw_ptr<Profile> profile_ = nullptr;
};

}  // namespace

OnionLocationView::OnionLocationView(Profile* profile) {
  SetBorder(views::CreateEmptyBorder(gfx::Insets::VH(1, 3)));
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
  if (!web_contents)
    return;
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
