/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/side_panel/brave_read_later_side_panel_view.h"

#include <memory>

#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/grit/brave_generated_resources.h"
#include "brave/grit/brave_theme_resources.h"
#include "chrome/browser/ui/views/side_panel/read_later_side_panel_web_view.h"
#include "chrome/browser/ui/views/side_panel/side_panel_content_proxy.h"
#include "chrome/browser/ui/views/side_panel/side_panel_util.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/background.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/separator.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/layout_types.h"

namespace {

// Renders icon and title.
class ReadLaterSidePanelHeaderView : public views::View {
 public:
  ReadLaterSidePanelHeaderView() {
    constexpr int kHeaderInteriorMargin = 16;
    SetLayoutManager(std::make_unique<views::FlexLayout>())
        ->SetOrientation(views::LayoutOrientation::kHorizontal)
        .SetInteriorMargin(gfx::Insets(kHeaderInteriorMargin))
        .SetMainAxisAlignment(views::LayoutAlignment::kStart)
        .SetCrossAxisAlignment(views::LayoutAlignment::kCenter);

    ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
    auto* header_image = AddChildView(
        std::make_unique<views::ImageView>(ui::ImageModel::FromImageSkia(
            *rb.GetImageSkiaNamed(IDR_SIDEBAR_READING_LIST_PANEL_HEADER))));
    constexpr int kSpacingBetweenHeaderImageAndLabel = 8;
    header_image->SetProperty(
        views::kMarginsKey,
        gfx::Insets::TLBR(0, 0, 0, kSpacingBetweenHeaderImageAndLabel));
    header_image->SetProperty(
        views::kFlexBehaviorKey,
        views::FlexSpecification(views::MinimumFlexSizeRule::kPreferred,
                                 views::MaximumFlexSizeRule::kPreferred));

    auto* header_label =
        AddChildView(std::make_unique<views::Label>(l10n_util::GetStringUTF16(
            IDS_SIDEBAR_READING_LIST_PANEL_HEADER_TITLE)));
    header_label->SetFontList(gfx::FontList("Poppins, Semi-Bold 16px"));
    header_label->SetEnabledColorId(kColorSidebarPanelHeaderTitle);
    header_label->SetProperty(
        views::kFlexBehaviorKey,
        views::FlexSpecification(views::MinimumFlexSizeRule::kPreferred,
                                 views::MaximumFlexSizeRule::kPreferred));

    SetBackground(
        views::CreateThemedSolidBackground(kColorSidebarPanelHeaderBackground));
  }

  ~ReadLaterSidePanelHeaderView() override = default;
  ReadLaterSidePanelHeaderView(const ReadLaterSidePanelHeaderView&) = delete;
  ReadLaterSidePanelHeaderView& operator=(const ReadLaterSidePanelHeaderView&) =
      delete;
};

}  // namespace

BraveReadLaterSidePanelView::BraveReadLaterSidePanelView(
    Browser* browser,
    base::RepeatingClosure close_cb) {
  SetLayoutManager(std::make_unique<views::FlexLayout>())
      ->SetOrientation(views::LayoutOrientation::kVertical);
  AddChildView(std::make_unique<ReadLaterSidePanelHeaderView>());
  AddChildView(std::make_unique<views::Separator>())
      ->SetColorId(kColorSidebarPanelHeaderSeparator);
  auto* web_view = AddChildView(
      std::make_unique<ReadLaterSidePanelWebView>(browser, close_cb));
  web_view->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kPreferred,
                               views::MaximumFlexSizeRule::kUnbounded));

  // Originally SidePanelEntry's Content was ReadLaterSidePanelWebView
  // and it's availability is set to true when SidePanelWebUIView::ShowUI()
  // and then proxy's availability callback is executed.
  // However, we use parent view(BraveReadLaterSidePanelView) to have
  // panel specific header view and this class becomes SidePanelEntry's Content.
  // To make this content available when SidePanelWebUIVew::ShowUI() is called,
  // this observes WebView's visibility because it's set as visible when
  // ShowUI() is called.
  // NOTE: If we use our own reading list page and it has loading spinner, maybe
  // we can set `true` here.
  SidePanelUtil::GetSidePanelContentProxy(this)->SetAvailable(false);
  observation_.Observe(web_view);
}

void BraveReadLaterSidePanelView::OnViewVisibilityChanged(
    views::View* observed_view,
    views::View* starting_view) {
  // Once it becomes available, stop observing becuase its availablity is
  // not changed from now on.
  if (observed_view->GetVisible()) {
    SidePanelUtil::GetSidePanelContentProxy(this)->SetAvailable(true);
    observation_.Reset();
  }
}

BraveReadLaterSidePanelView::~BraveReadLaterSidePanelView() = default;
