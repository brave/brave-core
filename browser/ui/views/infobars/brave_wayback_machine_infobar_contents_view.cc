/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/infobars/brave_wayback_machine_infobar_contents_view.h"

#include <memory>
#include <string>
#include <utility>

#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/themes/theme_properties.h"
#include "brave/browser/ui/views/infobars/brave_wayback_machine_infobar_button_container.h"
#include "brave/components/brave_wayback_machine/brave_wayback_machine_infobar_delegate.h"
#include "brave/components/brave_wayback_machine/wayback_machine_url_fetcher.h"
#include "brave/grit/brave_generated_resources.h"
#include "brave/grit/brave_theme_resources.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/browser/ui/views/chrome_typography.h"
#include "components/grit/components_scaled_resources.h"
#include "components/infobars/core/infobar.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/theme_provider.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/view_class_properties.h"
#include "url/gurl.h"

namespace {
// IDs of the colors to use for infobar elements.
constexpr int kInfoBarLabelBackgroundColor = ThemeProperties::COLOR_INFOBAR;
constexpr int kInfoBarLabelTextColor = ThemeProperties::COLOR_BOOKMARK_TEXT;
}  // namespace

BraveWaybackMachineInfoBarContentsView::BraveWaybackMachineInfoBarContentsView(
    content::WebContents* contents)
    : contents_(contents),
      wayback_machine_url_fetcher_(
          this,
          content::BrowserContext::GetDefaultStoragePartition(
              contents->GetBrowserContext())->
                  GetURLLoaderFactoryForBrowserProcess()) {
  SetLayoutManager(std::make_unique<views::FlexLayout>());
  InitializeChildren();
}

BraveWaybackMachineInfoBarContentsView::
~BraveWaybackMachineInfoBarContentsView() {}

void BraveWaybackMachineInfoBarContentsView::OnThemeChanged() {
  views::View::OnThemeChanged();

  const SkColor background_color = GetColor(kInfoBarLabelBackgroundColor);
  const SkColor text_color = GetColor(kInfoBarLabelTextColor);
  for (auto* label : labels_) {
    label->SetBackgroundColor(background_color);
    label->SetEnabledColor(text_color);
  }

  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  wayback_spot_graphic_->SetImage(
      rb.GetImageSkiaNamed(ui::NativeTheme::GetInstanceForNativeUi()->
          ShouldUseDarkColors() ? IDR_BRAVE_WAYBACK_INFOBAR_DARK
                                : IDR_BRAVE_WAYBACK_INFOBAR));
}

void BraveWaybackMachineInfoBarContentsView::ButtonPressed(
    views::Button* sender,
    const ui::Event& event) {
  if (wayback_url_fetch_requested_)
    return;
  wayback_url_fetch_requested_ = true;

  FetchWaybackURL();
}

void BraveWaybackMachineInfoBarContentsView::OnWaybackURLFetched(
    const GURL& latest_wayback_url) {
  DCHECK(wayback_url_fetch_requested_);
  wayback_url_fetch_requested_ = false;

  button_->StopThrobber();
  Layout();

  if (latest_wayback_url.is_empty()) {
    UpdateChildrenVisibility(false);
    return;
  }

  LoadURL(latest_wayback_url);
  // After loading to archived url, don't need to show infobar anymore.
  HideInfobar();
}

void BraveWaybackMachineInfoBarContentsView::HideInfobar() {
  InfoBarService* infobar_service = InfoBarService::FromWebContents(contents_);
  if (!infobar_service)
    return;

  for (size_t i = 0; i < infobar_service->infobar_count(); ++i) {
    infobars::InfoBar* infobar = infobar_service->infobar_at(i);
    if (infobar->delegate()->GetIdentifier() ==
        BraveWaybackMachineInfoBarDelegate::WAYBACK_MACHINE_INFOBAR_DELEGATE) {
      infobar_service->RemoveInfoBar(infobar);
      break;
    }
  }
}

void BraveWaybackMachineInfoBarContentsView::InitializeChildren() {
  wayback_spot_graphic_ = new views::ImageView();
  wayback_spot_graphic_->SetProperty(views::kMarginsKey,
                                    gfx::Insets(8, 34, 8, 24));
  AddChildView(wayback_spot_graphic_);

  const views::FlexSpecification label_flex_rule =
      views::FlexSpecification(views::MinimumFlexSizeRule::kScaleToMinimum,
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
      views::FlexSpecification(views::MinimumFlexSizeRule::kScaleToZero,
                               views::MaximumFlexSizeRule::kUnbounded)
          .WithOrder(3));
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

  button_ = new BraveWaybackMachineInfoBarButtonContainer(this);
  views_visible_before_checking_.push_back(button_);
  button_->SetProperty(
      views::kMarginsKey,
      gfx::Insets(ChromeLayoutProvider::Get()->GetDistanceMetric(
                      DISTANCE_TOAST_CONTROL_VERTICAL),
                  0));
  button_->SizeToPreferredSize();
  AddChildView(button_);

  UpdateChildrenVisibility(true);
}

views::Label* BraveWaybackMachineInfoBarContentsView::CreateLabel(
    const base::string16& text) {
  views::Label* label =
      new views::Label(text, views::style::CONTEXT_DIALOG_BODY_TEXT);
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
  button_->StartThrobber();
  wayback_machine_url_fetcher_.Fetch(contents_->GetVisibleURL());
  Layout();
}

void BraveWaybackMachineInfoBarContentsView::LoadURL(const GURL& url) {
  contents_->GetController().LoadURL(url,
                                     content::Referrer(),
                                     ui::PAGE_TRANSITION_LINK,
                                     std::string());
}
