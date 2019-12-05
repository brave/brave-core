/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/infobars/brave_wayback_machine_infobar_contents_view.h"

#include <memory>
#include <string>
#include <utility>

#include "brave/browser/brave_wayback_machine/wayback_machine_url_fetcher.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/browser/ui/views/chrome_typography.h"
#include "components/grit/components_scaled_resources.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/theme_provider.h"
#include "ui/gfx/color_palette.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/separator.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/view_class_properties.h"
#include "url/gurl.h"

namespace {
// IDs of the colors to use for infobar elements.
constexpr int kInfoBarLabelBackgroundColor = ThemeProperties::COLOR_INFOBAR;
constexpr int kInfoBarLabelTextColor = ThemeProperties::COLOR_BOOKMARK_TEXT;
}  // namespace

BraveWaybackMachineInfoBarContentsView::BraveWaybackMachineInfoBarContentsView(
    infobars::InfoBar* infobar,
    content::WebContents* contents)
    : infobar_(infobar),
      contents_(contents),
      wayback_machine_url_fetcher_(
          this,
          SystemNetworkContextManager::GetInstance()->
              GetSharedURLLoaderFactory()) {
  SetLayoutManager(std::make_unique<views::FlexLayout>());
  InitializeChildren();
}

BraveWaybackMachineInfoBarContentsView::
~BraveWaybackMachineInfoBarContentsView() {}

void BraveWaybackMachineInfoBarContentsView::OnThemeChanged() {
  const SkColor background_color = GetColor(kInfoBarLabelBackgroundColor);
  const SkColor text_color = GetColor(kInfoBarLabelTextColor);
  for (auto* label : labels_) {
    label->SetBackgroundColor(background_color);
    label->SetEnabledColor(text_color);
  }

  separator_->SetColor(text_color);
}

void BraveWaybackMachineInfoBarContentsView::ButtonPressed(
    views::Button* sender,
    const ui::Event& event) {
  FetchWaybackURL();
}

void BraveWaybackMachineInfoBarContentsView::OnWaybackURLFetched(
    const GURL& latest_wayback_url) {
  if (latest_wayback_url.is_empty()) {
    UpdateChildrenVisibility(false);
    return;
  }

  LoadURL(latest_wayback_url);
  // After loading to archived url, don't need to show infobar anymore.
  InfoBarService::FromWebContents(contents_)->RemoveInfoBar(infobar_);
}

void BraveWaybackMachineInfoBarContentsView::InitializeChildren() {
  // TODO(simonhong): Use real image assets.
  views::ImageView* image_view = new views::ImageView();
  image_view->SetImageSize(gfx::Size(100, 20));
  image_view->SetProperty(views::kMarginsKey,
                          gfx::Insets(12, 20, 12, 20));
  image_view->SetBackground(
      views::CreateSolidBackground(SkColorSetRGB(0xff, 0x76, 0x54)));
  AddChildView(image_view);

  separator_ = new views::Separator;
  separator_->SetProperty(views::kMarginsKey,
                          gfx::Insets(12, 0, 12, 20));
  AddChildView(separator_);

  const views::FlexSpecification label_flex_rule =
      views::FlexSpecification::ForSizeRule(
          views::MinimumFlexSizeRule::kScaleToMinimum,
          views::MaximumFlexSizeRule::kPreferred);
  auto* label = CreateLabel(
      l10n_util::GetStringUTF16(
          IDS_BRAVE_WAYBACK_MACHINE_INFOBAR_PAGE_MISSING_TEXT));
  label->SetFontList(
      label->font_list().DeriveWithWeight(gfx::Font::Weight::BOLD));
  views_visible_before_checking_.push_back(label);
  label->SetProperty(views::kFlexBehaviorKey, label_flex_rule.WithOrder(1));
  label->SetProperty(
      views::kMarginsKey,
      gfx::Insets(ChromeLayoutProvider::Get()->GetDistanceMetric(
                      DISTANCE_TOAST_LABEL_VERTICAL),
                  0));
  AddChildView(label);

  label = CreateLabel(l10n_util::GetStringUTF16(
      IDS_BRAVE_WAYBACK_MACHINE_INFOBAR_ASK_ABOUT_CHECK_TEXT));
  views_visible_before_checking_.push_back(label);
  label->SetProperty(
      views::kMarginsKey,
      gfx::Insets(ChromeLayoutProvider::Get()->GetDistanceMetric(
                      DISTANCE_TOAST_LABEL_VERTICAL),
                  5));
  label->SetElideBehavior(gfx::ELIDE_TAIL);
  label->SetProperty(views::kFlexBehaviorKey, label_flex_rule.WithOrder(2));
  AddChildView(label);

  // Add empty view to locate button to last.
  auto* place_holder_view = new views::View;
  views_visible_before_checking_.push_back(place_holder_view);
  place_holder_view->SetProperty(views::kMarginsKey, gfx::Insets(12, 0));
  place_holder_view->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification::ForSizeRule(
          views::MinimumFlexSizeRule::kScaleToZero,
          views::MaximumFlexSizeRule::kUnbounded).WithOrder(3));
  AddChildView(place_holder_view);

  label = CreateLabel(
      l10n_util::GetStringUTF16(
          IDS_BRAVE_WAYBACK_MACHINE_INFOBAR_NOT_AVAILABLE_TEXT));
  views_visible_after_checking_.push_back(label);
  label->SetProperty(views::kFlexBehaviorKey, label_flex_rule);
  label->SetProperty(
      views::kMarginsKey,
      gfx::Insets(ChromeLayoutProvider::Get()->GetDistanceMetric(
                      DISTANCE_TOAST_LABEL_VERTICAL),
                  0));
  AddChildView(label);

  views::ImageView* sad_icon = new views::ImageView();
  views_visible_after_checking_.push_back(sad_icon);
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  sad_icon->SetImage(rb.GetImageSkiaNamed(IDR_CRASH_SAD_FAVICON));
  sad_icon->SetProperty(views::kMarginsKey, gfx::Insets(12, 10));
  AddChildView(sad_icon);

  auto button = views::MdTextButton::CreateSecondaryUiBlueButton(
      this,
      l10n_util::GetStringUTF16(IDS_BRAVE_WAYBACK_MACHINE_CHECK_BUTTON_TEXT));
  auto* button_ptr = button.get();
  views_visible_before_checking_.push_back(button_ptr);
  button->SetProperty(
      views::kMarginsKey,
      gfx::Insets(ChromeLayoutProvider::Get()->GetDistanceMetric(
                      DISTANCE_TOAST_CONTROL_VERTICAL),
                  0));
  button->SizeToPreferredSize();
  AddChildView(button.release());

  UpdateChildrenVisibility(true);
}

views::Label* BraveWaybackMachineInfoBarContentsView::CreateLabel(
    const base::string16& text) {
  views::Label* label = new views::Label(text, CONTEXT_BODY_TEXT_LARGE);
  labels_.push_back(label);
  label->SetBackgroundColor(GetColor(kInfoBarLabelBackgroundColor));
  label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  label->SetEnabledColor(GetColor(kInfoBarLabelTextColor));
  return label;
}

void BraveWaybackMachineInfoBarContentsView::UpdateChildrenVisibility(
    bool show_before_checking_views) {
  for (auto* view : views_visible_before_checking_)
    view->SetVisible(show_before_checking_views);
  for (auto* view : views_visible_after_checking_)
    view->SetVisible(!show_before_checking_views);
}

SkColor BraveWaybackMachineInfoBarContentsView::GetColor(int id) const {
  const auto* theme_provider = GetThemeProvider();
  return theme_provider ? theme_provider->GetColor(id)
                        : gfx::kPlaceholderColor;
}

void BraveWaybackMachineInfoBarContentsView::FetchWaybackURL() {
  wayback_machine_url_fetcher_.Fetch(contents_->GetVisibleURL());
}

void BraveWaybackMachineInfoBarContentsView::LoadURL(const GURL& url) {
  contents_->GetController().LoadURL(url,
                                     content::Referrer(),
                                     ui::PAGE_TRANSITION_LINK,
                                     std::string());
}
