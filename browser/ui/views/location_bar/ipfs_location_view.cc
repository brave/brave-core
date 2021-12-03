/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/location_bar/ipfs_location_view.h"

#include <memory>
#include <utility>

#include "base/memory/raw_ptr.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/ipfs/ipfs_tab_helper.h"
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

constexpr SkColor kOpenInIpfsBg = SkColorSetRGB(0x6a, 0x37, 0x85);
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

class IPFSLocationButtonView : public views::LabelButton {
 public:
  explicit IPFSLocationButtonView(Profile* profile)
      : LabelButton(
            base::BindRepeating(&IPFSLocationButtonView::ButtonPressed,
                                base::Unretained(this)),
            l10n_util::GetStringUTF16(IDS_LOCATION_BAR_OPEN_USING_IPFS)),
        profile_(profile) {
    // Render vector icon
    const gfx::ImageSkia image =
        gfx::CreateVectorIcon(kOpenInIpfsIcon, kIconSize, kIconColor);
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

  IPFSLocationButtonView(const IPFSLocationButtonView&) = delete;
  IPFSLocationButtonView& operator=(const IPFSLocationButtonView&) = delete;

  ~IPFSLocationButtonView() override {}

  void SetIPFSLocation(GURL location) { ipfs_location_ = location; }

 private:
  // views::View
  void Layout() override {
    views::LabelButton::Layout();
    UpdateBorder();
  }

  void UpdateBorder() {
    SetBackground(
        views::CreateRoundedRectBackground(kOpenInIpfsBg, height() / 2));
  }

  void ButtonPressed() {
    Browser* browser = chrome::FindTabbedBrowser(profile_, true);
    if (!browser)
      return;
    content::OpenURLParams open_ipfs(ipfs_location_, content::Referrer(),
                                     WindowOpenDisposition::NEW_FOREGROUND_TAB,
                                     ui::PAGE_TRANSITION_TYPED, false);
    browser->OpenURL(open_ipfs);
  }

  GURL ipfs_location_;
  raw_ptr<Profile> profile_ = nullptr;
};

}  // namespace

IPFSLocationView::IPFSLocationView(Profile* profile) {
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

  button_ = new IPFSLocationButtonView(profile);
  AddChildView(button_);
}

IPFSLocationView::~IPFSLocationView() {}

void IPFSLocationView::Update(content::WebContents* web_contents) {
  if (!web_contents)
    return;
  ipfs::IPFSTabHelper* helper =
      ipfs::IPFSTabHelper::FromWebContents(web_contents);
  if (!helper)
    return;
  auto ipfs_resolved_url = helper->GetIPFSResolvedURL();
  SetVisible(ipfs_resolved_url.is_valid());
  reinterpret_cast<IPFSLocationButtonView*>(button_)->SetIPFSLocation(
      ipfs_resolved_url);
}
