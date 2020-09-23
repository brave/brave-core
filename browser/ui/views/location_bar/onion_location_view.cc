/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/location_bar/onion_location_view.h"

#include <memory>
#include <utility>

#include "base/strings/utf_string_conversions.h"
#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/tor/onion_location_tab_helper.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/views/toolbar/toolbar_ink_drop_util.h"
#include "components/grit/brave_components_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/image_model.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/geometry/size.h"
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

void OnTorProfileCreated(GURL onion_location,
                         Profile* profile,
                         Profile::CreateStatus status) {
  if (status != Profile::CreateStatus::CREATE_STATUS_INITIALIZED)
    return;
  Browser* browser = chrome::FindTabbedBrowser(profile, true);
  if (!browser)
    return;
  content::OpenURLParams open_tor(onion_location, content::Referrer(),
                                  WindowOpenDisposition::OFF_THE_RECORD,
                                  ui::PAGE_TRANSITION_TYPED, false);
  browser->OpenURL(open_tor);
}

// Sets the focus and ink drop highlight path to match the background
// along with it's corner radius.
class HighlightPathGenerator : public views::HighlightPathGenerator {
 public:
  HighlightPathGenerator() = default;

  // views::HighlightPathGenerator:
  SkPath GetHighlightPath(const views::View* view) override {
    const gfx::Rect highlight_bounds = view->GetLocalBounds();
    const SkRect rect = RectToSkRect(highlight_bounds);
    const int corner_radius = view->height() / 2;
    return SkPath().addRoundRect(rect, corner_radius, corner_radius);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(HighlightPathGenerator);
};

class OnionLocationButtonView : public views::LabelButton,
                                public views::ButtonListener {
 public:
  explicit OnionLocationButtonView(Profile* profile)
      : LabelButton(this,
                    l10n_util::GetStringUTF16(IDS_LOCATION_BAR_OPEN_IN_TOR)) {
    if (brave::IsTorProfile(profile))
      SetText(l10n_util::GetStringUTF16(IDS_LOCATION_BAR_ONION_AVAILABLE));
    // Render vector icon
    const gfx::ImageSkia image =
        gfx::CreateVectorIcon(kOpenInTorIcon, kIconSize, kIconColor);
    SetImageModel(views::Button::STATE_NORMAL,
                  ui::ImageModel::FromImageSkia(image));
    // Set style specifics
    SetEnabledTextColors(kTextColor);
    SetHorizontalAlignment(gfx::ALIGN_RIGHT);
    SetImageLabelSpacing(6);
    SetInkDropMode(InkDropMode::ON);
    SetBorder(views::CreateEmptyBorder(
        GetLayoutInsets(LOCATION_BAR_ICON_INTERIOR_PADDING)));
    set_has_ink_drop_action_on_click(true);
    SetInkDropVisibleOpacity(kToolbarInkDropVisibleOpacity);
    UpdateBorder();
    // Ensure focus ring follows border
    views::HighlightPathGenerator::Install(
        this, std::make_unique<HighlightPathGenerator>());
  }

  ~OnionLocationButtonView() override {}

  // views::ButtonListener
  void ButtonPressed(Button* sender, const ui::Event& event) override {
    profiles::SwitchToTorProfile(
        base::BindRepeating(&OnTorProfileCreated, GURL(onion_location_)));
  }

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

  GURL onion_location_;

  OnionLocationButtonView(const OnionLocationButtonView&) = delete;
  OnionLocationButtonView& operator=(const OnionLocationButtonView&) = delete;
};

}  // namespace

OnionLocationView::OnionLocationView(Profile* profile) {
  SetBorder(views::CreateEmptyBorder(gfx::Insets(3, 3)));
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
  AddChildView(button_);
}

OnionLocationView::~OnionLocationView() {}

void OnionLocationView::Update(content::WebContents* web_contents) {
  if (!web_contents)
    return;
  tor::OnionLocationTabHelper* helper =
      tor::OnionLocationTabHelper::FromWebContents(web_contents);
  if (!helper)
    return;
  SetVisible(helper->should_show_icon());
  reinterpret_cast<OnionLocationButtonView*>(button_)->SetOnionLocation(
      helper->onion_location());
}
