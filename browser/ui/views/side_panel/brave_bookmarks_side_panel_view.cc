/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/side_panel/brave_bookmarks_side_panel_view.h"

#include <memory>

#include "base/memory/raw_ref.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "brave/grit/brave_generated_resources.h"
#include "brave/grit/brave_theme_resources.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/singleton_tabs.h"
#include "chrome/browser/ui/views/side_panel/bookmarks/bookmarks_side_panel_coordinator.h"
#include "chrome/browser/ui/views/side_panel/read_later_side_panel_web_view.h"
#include "chrome/browser/ui/views/side_panel/side_panel_ui.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/models/image_model.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/separator.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/layout_types.h"

namespace {

// Renders icon, title and launch button.
class BookmarksSidePanelHeaderView : public views::View {
  METADATA_HEADER(BookmarksSidePanelHeaderView, views::View)

 public:
  explicit BookmarksSidePanelHeaderView(Browser* browser) {
    constexpr int kHeaderInteriorMargin = 16;
    SetLayoutManager(std::make_unique<views::FlexLayout>())
        ->SetOrientation(views::LayoutOrientation::kHorizontal)
        .SetInteriorMargin(gfx::Insets(kHeaderInteriorMargin))
        .SetMainAxisAlignment(views::LayoutAlignment::kStart)
        .SetCrossAxisAlignment(views::LayoutAlignment::kCenter);

    ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
    auto* header_image = AddChildView(
        std::make_unique<views::ImageView>(ui::ImageModel::FromImageSkia(
            *rb.GetImageSkiaNamed(IDR_SIDEBAR_BOOKMARKS_PANEL_HEADER))));
    constexpr int kSpacingBetweenHeaderImageAndLabel = 8;
    header_image->SetProperty(
        views::kMarginsKey,
        gfx::Insets::TLBR(0, 0, 0, kSpacingBetweenHeaderImageAndLabel));
    header_image->SetProperty(
        views::kFlexBehaviorKey,
        views::FlexSpecification(views::MinimumFlexSizeRule::kPreferred,
                                 views::MaximumFlexSizeRule::kPreferred));

    auto* header_label = AddChildView(std::make_unique<views::Label>(
        l10n_util::GetStringUTF16(IDS_BOOKMARK_MANAGER_TITLE)));
    header_label->SetFontList(gfx::FontList("Poppins, Semi-Bold 16px"));
    header_label->SetEnabledColorId(kColorSidebarPanelHeaderTitle);
    header_label->SetAutoColorReadabilityEnabled(false);
    auto* spacer = AddChildView(std::make_unique<views::View>());
    spacer->SetProperty(
        views::kFlexBehaviorKey,
        views::FlexSpecification(views::MinimumFlexSizeRule::kScaleToZero,
                                 views::MaximumFlexSizeRule::kUnbounded)
            .WithOrder(2));
    auto* button =
        AddChildView(std::make_unique<views::ImageButton>(base::BindRepeating(
            [](Browser* browser, const ui::Event& event) {
              ShowSingletonTab(browser, GURL(chrome::kChromeUIBookmarksURL));
            },
            browser)));
    button->SetTooltipText(l10n_util::GetStringUTF16(
        IDS_SIDEBAR_READING_LIST_PANEL_HEADER_BOOKMARKS_BUTTON_TOOLTIP));

    constexpr int kHeaderButtonSize = 20;
    button->SetImageModel(
        views::Button::STATE_NORMAL,
        ui::ImageModel::FromVectorIcon(
            kLeoLaunchIcon, kColorSidebarPanelHeaderButton, kHeaderButtonSize));
    button->SetImageModel(
        views::Button::STATE_HOVERED,
        ui::ImageModel::FromVectorIcon(kLeoLaunchIcon,
                                       kColorSidebarPanelHeaderButtonHovered,
                                       kHeaderButtonSize));

    auto* separator = AddChildView(std::make_unique<views::Separator>());
    separator->SetColorId(kColorSidebarPanelHeaderSeparator);
    separator->SetPreferredLength(kHeaderButtonSize);
    constexpr int kSeparatorHorizontalSpacing = 12;
    separator->SetProperty(views::kMarginsKey,
                           gfx::Insets::VH(0, kSeparatorHorizontalSpacing));

    button =
        AddChildView(std::make_unique<views::ImageButton>(base::BindRepeating(
            [](Browser* browser) {
              if (SidePanelUI* ui = browser->GetFeatures().side_panel_ui()) {
                ui->Close();
              }
            },
            browser)));
    button->SetTooltipText(
        l10n_util::GetStringUTF16(IDS_SIDEBAR_PANEL_CLOSE_BUTTON_TOOLTIP));
    button->SetImageModel(
        views::Button::STATE_NORMAL,
        ui::ImageModel::FromVectorIcon(
            kLeoCloseIcon, kColorSidebarPanelHeaderButton, kHeaderButtonSize));
    button->SetImageModel(
        views::Button::STATE_HOVERED,
        ui::ImageModel::FromVectorIcon(kLeoCloseIcon,
                                       kColorSidebarPanelHeaderButtonHovered,
                                       kHeaderButtonSize));
  }

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override {
    if (available_size.is_fully_bounded()) {
      return {available_size.width().value(),
              BraveSidePanelViewBase::kHeaderHeight};
    }

    return View::CalculatePreferredSize(available_size);
  }

  ~BookmarksSidePanelHeaderView() override = default;
  BookmarksSidePanelHeaderView(const BookmarksSidePanelHeaderView&) = delete;
  BookmarksSidePanelHeaderView& operator=(const BookmarksSidePanelHeaderView&) =
      delete;
};

BEGIN_METADATA(BookmarksSidePanelHeaderView)
END_METADATA

}  // namespace

BraveBookmarksSidePanelView::BraveBookmarksSidePanelView(
    Browser* browser,
    SidePanelEntryScope& scope) {
  SetLayoutManager(std::make_unique<views::FlexLayout>())
      ->SetOrientation(views::LayoutOrientation::kVertical);
  AddChildView(std::make_unique<BookmarksSidePanelHeaderView>(browser));
  AddChildView(std::make_unique<views::Separator>())
      ->SetColorId(kColorSidebarPanelHeaderSeparator);

  // Reuse upstream's bookmarks panl nwebui.
  auto* web_view =
      AddChildView(BookmarksSidePanelCoordinator::GetOrCreateForBrowser(browser)
                       ->CreateBookmarksWebView(scope));
  web_view->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kPreferred,
                               views::MaximumFlexSizeRule::kUnbounded));

  StartObservingWebWebViewVisibilityChange(web_view);
}

BraveBookmarksSidePanelView::~BraveBookmarksSidePanelView() = default;
