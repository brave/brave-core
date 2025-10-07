/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/page_info/brave_page_info_bubble_view.h"

#include "brave/browser/ui/page_info/features.h"
#include "brave/browser/ui/views/page_info/brave_shields_page_info_view.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "chrome/browser/ui/views/page_info/page_info_view_factory.h"
#include "components/grit/brave_components_strings.h"
#include "components/strings/grit/components_strings.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/navigation_handle.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/models/image_model.h"
#include "ui/color/color_id.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/separator.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/view_class_properties.h"

namespace {

constexpr ui::ColorId kTabButtonColor = ui::kColorTabForeground;
constexpr ui::ColorId kTabButtonHighlightColor =
    ui::kColorTabForegroundSelected;

std::pair<int, const gfx::VectorIcon&> GetTabButtonTextAndIcon(
    BravePageInfoBubbleView::Tab tab) {
  switch (tab) {
    case BravePageInfoBubbleView::Tab::kShields:
      return {IDS_BRAVE_SHIELDS, kLeoShieldDoneIcon};
    case BravePageInfoBubbleView::Tab::kSiteSettings:
      return {IDS_PAGE_INFO_SITE_SETTINGS_LINK, kLeoTuneSmallIcon};
  }
}

}  // namespace

BravePageInfoBubbleView::~BravePageInfoBubbleView() = default;

void BravePageInfoBubbleView::InitializeView() {
  if (!page_info::features::IsShowBraveShieldsInPageInfoEnabled()) {
    return;
  }

  CustomizeChromiumViews();

  // Remove the top margin set by the parent class.
  set_margins(gfx::Insets());

  // Add the tab switcher at the top of the bubble.
  tab_switcher_ = AddChildViewAt(CreateTabSwitcher(), 0);

  // Add the Brave Shields view.
  auto* tab_interface = tabs::TabInterface::GetFromContents(web_contents());
  auto* browser_window_interface = tab_interface->GetBrowserWindowInterface();
  shields_page_view_ = AddChildView(std::make_unique<BraveShieldsPageInfoView>(
      browser_window_interface, web_contents()));

  // If the PageInfo bubble was not opened directly to a subpage, then show the
  // Shields tab first.
  if (!IsSiteSettingsSubpageActive()) {
    current_tab_ = Tab::kShields;
  }

  UpdateContentVisibilityForCurrentTab();
  SizeToContents();
}

void BravePageInfoBubbleView::OpenMainPage(
    base::OnceClosure initialized_callback) {
  PageInfoBubbleView::OpenMainPage(std::move(initialized_callback));
  CustomizeChromiumViews();
  SizeToContents();
}

void BravePageInfoBubbleView::AnnouncePageOpened(std::u16string announcement) {
  // This method is called after any PageInfo subpage is opened and allows us to
  // customize child views added by the superclass.
  PageInfoBubbleView::AnnouncePageOpened(std::move(announcement));
  CustomizeChromiumViews();
  SizeToContents();
}

void BravePageInfoBubbleView::Layout(PassKey pass_key) {
  LayoutSuperclass<PageInfoBubbleView>(this);

  if (page_info::features::IsShowBraveShieldsInPageInfoEnabled()) {
    UpdateTabIndicator();
  }
}

gfx::Size BravePageInfoBubbleView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  gfx::Size size = PageInfoBubbleView::CalculatePreferredSize(available_size);

  if (page_info::features::IsShowBraveShieldsInPageInfoEnabled()) {
    // This bubble needs to be larger than the parent class in order to show the
    // full tab switcher and Shields content.
    constexpr int kMinBubbleWidth = 388;
    size.set_width(std::max(size.width(), kMinBubbleWidth));
  }

  return size;
}

void BravePageInfoBubbleView::ChildPreferredSizeChanged(View* child) {
  // When child preferred sizes change (e.g. when the Shields WebView is auto
  // resized), we need to make sure that layout caches are dropped before
  // calculating the size of the bubble.
  InvalidateLayout();
  SizeToContents();
}

void BravePageInfoBubbleView::PrimaryPageChanged(content::Page& page) {
  // The superclass closes the bubble when this event occurs. Since we are
  // displaying the Shields UI and we want users to be able to toggle Shields
  // settings without closing the bubble.
}

void BravePageInfoBubbleView::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  // Returns true if the page info bubble should be closed for this navigation.
  // Although we want to leave the bubble open when the user toggles Shields
  // settings, we must be careful to not allow the superclass to display stale
  // site information.
  auto should_close_bubble = [&]() {
    // We can ignore any navigation that is not a primary page change.
    const bool is_page_change = navigation_handle->IsInPrimaryMainFrame() &&
                                !navigation_handle->IsSameDocument() &&
                                navigation_handle->HasCommitted();
    if (!is_page_change) {
      return false;
    }

    // Close the bubble if the Shields integration flag is not enabled.
    if (!page_info::features::IsShowBraveShieldsInPageInfoEnabled()) {
      return true;
    }

    // Always close the bubble if this is a cross-origin navigation, regardless
    // of any other considerations.
    if (!navigation_handle->IsSameOrigin()) {
      return true;
    }

    // We can leave the bubble open if this is a reload (e.g. the user has
    // made a change to Shields settings for the current tab).
    if (navigation_handle->GetReloadType() != content::ReloadType::NONE) {
      return false;
    }

    return true;
  };

  if (should_close_bubble()) {
    GetWidget()->Close();
  }
}

void BravePageInfoBubbleView::CustomizeChromiumViews() {
  if (!page_info::features::IsShowBraveShieldsInPageInfoEnabled()) {
    return;
  }

  // Hide the close button in the page header for the main page or any subpage.
  auto* close_button =
      GetViewByID(PageInfoViewFactory::VIEW_ID_PAGE_INFO_CLOSE_BUTTON);
  if (close_button) {
    close_button->SetVisible(false);
  }

  // Find the first site settings content child and set its top margin.
  for (views::View* child : children()) {
    if (IsSiteSettingsChildView(child)) {
      const int top_margin = IsSiteSettingsSubpageActive() ? 16 : 8;
      child->SetProperty(views::kMarginsKey, gfx::Insets().set_top(top_margin));
      break;
    }
  }
}

std::unique_ptr<views::View> BravePageInfoBubbleView::CreateTabSwitcher() {
  auto container = std::make_unique<views::View>();

  // Use vertical layout to stack buttons and separator.
  container->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));

  // Create a container for the tab buttons with horizontal flex layout.
  auto button_container = std::make_unique<views::View>();
  auto* layout =
      button_container->SetLayoutManager(std::make_unique<views::FlexLayout>());
  layout->SetOrientation(views::LayoutOrientation::kHorizontal)
      .SetMainAxisAlignment(views::LayoutAlignment::kCenter)
      .SetCrossAxisAlignment(views::LayoutAlignment::kStretch)
      .SetInteriorMargin(gfx::Insets::VH(0, 16));

  // Add the "Brave Shields" button.
  shields_button_ =
      button_container->AddChildView(CreateTabButton(Tab::kShields));

  // Add the "Site Settings" button.
  site_settings_button_ =
      button_container->AddChildView(CreateTabButton(Tab::kSiteSettings));

  // Add the button container to the main container.
  container->AddChildView(std::move(button_container));

  // Create the tab indicator bar (positioned manually under the active tab).
  // This view is excluded from layout management so we can position it freely.
  auto indicator = std::make_unique<views::View>();
  indicator->SetBackground(
      views::CreateSolidBackground(kTabButtonHighlightColor));
  indicator->SetProperty(views::kViewIgnoredByLayoutKey, true);
  tab_indicator_ = container->AddChildView(std::move(indicator));

  // Add a separator below the buttons.
  container->AddChildView(std::make_unique<views::Separator>());

  // Set the initial button styles.
  UpdateAllTabButtonStyles();

  return container;
}

std::unique_ptr<views::LabelButton> BravePageInfoBubbleView::CreateTabButton(
    Tab tab) {
  auto [text_id, icon] = GetTabButtonTextAndIcon(tab);

  auto button = std::make_unique<views::LabelButton>(
      base::BindRepeating(&BravePageInfoBubbleView::SwitchToTab,
                          base::Unretained(this), tab),
      l10n_util::GetStringUTF16(text_id));
  button->SetImageModel(
      views::Button::STATE_NORMAL,
      ui::ImageModel::FromVectorIcon(icon, kTabButtonColor, 16));
  button->SetImageLabelSpacing(8);
  button->SetLabelStyle(views::style::STYLE_HEADLINE_5);
  button->SetBorder(views::CreateEmptyBorder(gfx::Insets(16)));
  button->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  button->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::LayoutOrientation::kHorizontal,
                               views::MinimumFlexSizeRule::kScaleToZero,
                               views::MaximumFlexSizeRule::kUnbounded)
          .WithWeight(1));

  return button;
}

void BravePageInfoBubbleView::SwitchToTab(Tab tab) {
  current_tab_ = tab;
  UpdateContentVisibilityForCurrentTab();
  UpdateAllTabButtonStyles();
  UpdateTabIndicator();
  SizeToContents();
}

void BravePageInfoBubbleView::UpdateContentVisibilityForCurrentTab() {
  // Show/hide the Brave Shields page.
  CHECK(shields_page_view_);
  shields_page_view_->SetVisible(current_tab_ == Tab::kShields);

  // Show/hide Chromium page info content.
  for (views::View* child : children()) {
    if (IsSiteSettingsChildView(child)) {
      child->SetVisible(current_tab_ == Tab::kSiteSettings);
    }
  }
}

void BravePageInfoBubbleView::UpdateAllTabButtonStyles() {
  UpdateTabButtonStyles(Tab::kSiteSettings);
  UpdateTabButtonStyles(Tab::kShields);
}

void BravePageInfoBubbleView::UpdateTabButtonStyles(Tab tab) {
  auto* button = GetButtonForTab(tab);
  const ui::ColorId color =
      tab == current_tab_ ? kTabButtonHighlightColor : kTabButtonColor;

  // Update text colors.
  button->SetEnabledTextColors(color);
  button->SetTextColor(views::Button::STATE_HOVERED, kTabButtonHighlightColor);

  // Update icon colors.
  auto [text_id, icon] = GetTabButtonTextAndIcon(tab);
  button->SetImageModel(views::Button::STATE_NORMAL,
                        ui::ImageModel::FromVectorIcon(icon, color, 16));
  button->SetImageModel(
      views::Button::STATE_HOVERED,
      ui::ImageModel::FromVectorIcon(icon, kTabButtonHighlightColor, 16));
}

views::LabelButton* BravePageInfoBubbleView::GetButtonForTab(Tab tab) {
  switch (tab) {
    case Tab::kShields:
      return shields_button_.get();
    case Tab::kSiteSettings:
      return site_settings_button_.get();
  }
}

void BravePageInfoBubbleView::UpdateTabIndicator() {
  CHECK(tab_indicator_);

  constexpr int kIndicatorHeight = 2;

  views::LabelButton* active_button = GetButtonForTab(current_tab_);
  CHECK(active_button);

  // Convert button bounds to indicator's parent coordinate space.
  gfx::Rect button_bounds = active_button->bounds();
  views::View::ConvertRectToTarget(active_button->parent(),
                                   tab_indicator_->parent(), button_bounds);

  // Position the indicator under the active button with full button width.
  tab_indicator_->SetBounds(button_bounds.x(),
                            button_bounds.bottom() - kIndicatorHeight,
                            button_bounds.width(), kIndicatorHeight);
}

bool BravePageInfoBubbleView::IsSiteSettingsChildView(views::View* view) const {
  return view->parent() == this && view != tab_switcher_ &&
         view != shields_page_view_;
}

bool BravePageInfoBubbleView::IsSiteSettingsSubpageActive() const {
  // If a back button exists in the view tree, then we known that the view is
  // displaying a subpage.
  auto* back_button =
      GetViewByID(PageInfoViewFactory::VIEW_ID_PAGE_INFO_BACK_BUTTON);
  return static_cast<bool>(back_button);
}

BEGIN_METADATA(BravePageInfoBubbleView)
END_METADATA
