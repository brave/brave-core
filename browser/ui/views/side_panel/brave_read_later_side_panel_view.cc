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
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/base/metadata/metadata_impl_macros.h"
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
  METADATA_HEADER(ReadLaterSidePanelHeaderView, views::View)

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
    header_label->SetAutoColorReadabilityEnabled(false);
    header_label->SetProperty(
        views::kFlexBehaviorKey,
        views::FlexSpecification(views::MinimumFlexSizeRule::kPreferred,
                                 views::MaximumFlexSizeRule::kPreferred));
  }

  ~ReadLaterSidePanelHeaderView() override = default;
  ReadLaterSidePanelHeaderView(const ReadLaterSidePanelHeaderView&) = delete;
  ReadLaterSidePanelHeaderView& operator=(const ReadLaterSidePanelHeaderView&) =
      delete;
};

BEGIN_METADATA(ReadLaterSidePanelHeaderView)
END_METADATA

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

  StartObservingWebWebViewVisibilityChange(web_view);
}

BraveReadLaterSidePanelView::~BraveReadLaterSidePanelView() = default;

BEGIN_METADATA(BraveReadLaterSidePanelView)
END_METADATA
