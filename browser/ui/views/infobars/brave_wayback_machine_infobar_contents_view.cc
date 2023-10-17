/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/infobars/brave_wayback_machine_infobar_contents_view.h"

#include <memory>
#include <string>
#include <utility>

#include "base/ranges/algorithm.h"
#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/ui/views/infobars/brave_wayback_machine_infobar_button_container.h"
#include "brave/components/brave_wayback_machine/brave_wayback_machine_infobar_delegate.h"
#include "brave/components/brave_wayback_machine/pref_names.h"
#include "brave/components/brave_wayback_machine/wayback_machine_url_fetcher.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/grit/brave_generated_resources.h"
#include "brave/grit/brave_theme_resources.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/browser/ui/views/chrome_typography.h"
#include "components/grit/components_scaled_resources.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "components/infobars/core/infobar.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/theme_provider.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/controls/button/checkbox.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/view_class_properties.h"
#include "url/gurl.h"

namespace {
// IDs of the colors to use for infobar elements.
constexpr int kInfoBarLabelBackgroundColor = kColorInfoBarBackground;
constexpr int kInfoBarLabelTextColor = kColorBookmarkBarForeground;

// Subclass for custom font.
class DontAskAgainCheckbox : public views::Checkbox {
 public:
  METADATA_HEADER(DontAskAgainCheckbox);

  using views::Checkbox::Checkbox;
  ~DontAskAgainCheckbox() override = default;

  void SetFontList(const gfx::FontList& font_list) {
    label()->SetFontList(font_list);
  }
};

BEGIN_METADATA(DontAskAgainCheckbox, views::Checkbox)
END_METADATA
}  // namespace

BraveWaybackMachineInfoBarContentsView::BraveWaybackMachineInfoBarContentsView(
    content::WebContents* contents)
    : contents_(contents),
      wayback_machine_url_fetcher_(
          this,
          contents->GetBrowserContext()
              ->GetDefaultStoragePartition()
              ->GetURLLoaderFactoryForBrowserProcess()),
      pref_service_(
          user_prefs::UserPrefs::Get(contents_->GetBrowserContext())) {
  SetLayoutManager(std::make_unique<views::FlexLayout>());
  InitializeChildren();
}

BraveWaybackMachineInfoBarContentsView::
    ~BraveWaybackMachineInfoBarContentsView() = default;

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

void BraveWaybackMachineInfoBarContentsView::OnWaybackURLFetched(
    const GURL& latest_wayback_url) {
  DCHECK(wayback_url_fetch_requested_);
  wayback_url_fetch_requested_ = false;

  fetch_url_button_->StopThrobber();
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
  infobars::ContentInfoBarManager* infobar_manager =
      infobars::ContentInfoBarManager::FromWebContents(contents_);
  if (!infobar_manager)
    return;

  const auto it = base::ranges::find(
      infobar_manager->infobars(),
      BraveWaybackMachineInfoBarDelegate::WAYBACK_MACHINE_INFOBAR_DELEGATE,
      &infobars::InfoBar::GetIdentifier);
  if (it != infobar_manager->infobars().cend()) {
    infobar_manager->RemoveInfoBar(*it);
  }
}

void BraveWaybackMachineInfoBarContentsView::FetchURLButtonPressed() {
  if (wayback_url_fetch_requested_)
    return;
  wayback_url_fetch_requested_ = true;
  FetchWaybackURL();
}

void BraveWaybackMachineInfoBarContentsView::OnCheckboxUpdated() {
  pref_service_->SetBoolean(kBraveWaybackMachineEnabled,
                            !dont_ask_again_checkbox_->GetChecked());
}

void BraveWaybackMachineInfoBarContentsView::InitializeChildren() {
  wayback_spot_graphic_ = new views::ImageView();
  wayback_spot_graphic_->SetProperty(views::kMarginsKey,
                                     gfx::Insets::TLBR(8, 34, 8, 24));
  AddChildView(wayback_spot_graphic_.get());

  const views::FlexSpecification label_flex_rule =
      views::FlexSpecification(views::MinimumFlexSizeRule::kScaleToMinimum,
                               views::MaximumFlexSizeRule::kPreferred);
  auto* label = CreateLabel(brave_l10n::GetLocalizedResourceUTF16String(
      IDS_BRAVE_WAYBACK_MACHINE_INFOBAR_PAGE_MISSING_TEXT));
  label->SetFontList(
      label->font_list().DeriveWithWeight(gfx::Font::Weight::BOLD));
  views_visible_before_checking_.push_back(label);
  label->SetProperty(views::kFlexBehaviorKey, label_flex_rule);
  label->SetProperty(
      views::kMarginsKey,
      gfx::Insets::VH(ChromeLayoutProvider::Get()->GetDistanceMetric(
                          DISTANCE_TOAST_LABEL_VERTICAL),
                      0));
  AddChildView(label);

  label = CreateLabel(brave_l10n::GetLocalizedResourceUTF16String(
      IDS_BRAVE_WAYBACK_MACHINE_INFOBAR_ASK_ABOUT_CHECK_TEXT));
  views_visible_before_checking_.push_back(label);
  label->SetProperty(
      views::kMarginsKey,
      gfx::Insets::VH(ChromeLayoutProvider::Get()->GetDistanceMetric(
                          DISTANCE_TOAST_LABEL_VERTICAL),
                      5));
  label->SetElideBehavior(gfx::ELIDE_TAIL);
  label->SetProperty(views::kFlexBehaviorKey, label_flex_rule.WithOrder(2));
  AddChildView(label);

  dont_ask_again_checkbox_ =
      AddChildView(std::make_unique<DontAskAgainCheckbox>(
          brave_l10n::GetLocalizedResourceUTF16String(
              IDS_BRAVE_WAYBACK_MACHINE_DONT_ASK_AGAIN_TEXT),
          base::BindRepeating(
              &BraveWaybackMachineInfoBarContentsView::OnCheckboxUpdated,
              base::Unretained(this))));
  dont_ask_again_checkbox_->SetProperty(views::kMarginsKey,
                                        gfx::Insets::TLBR(12, 20, 12, 0));
  dont_ask_again_checkbox_->SetProperty(views::kFlexBehaviorKey,
                                        label_flex_rule);

  // Use same font with label. Checkbox's default font size is a little bit
  // smaller than label.
  static_cast<DontAskAgainCheckbox*>(dont_ask_again_checkbox_)
      ->SetFontList(label->font_list());
  views_visible_before_checking_.push_back(dont_ask_again_checkbox_);

  // Add empty view to locate button to last.
  auto* place_holder_view = new views::View;
  views_visible_before_checking_.push_back(place_holder_view);
  place_holder_view->SetProperty(views::kMarginsKey, gfx::Insets::VH(12, 0));
  place_holder_view->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kScaleToZero,
                               views::MaximumFlexSizeRule::kUnbounded)
          .WithOrder(3));
  AddChildView(place_holder_view);

  label = CreateLabel(brave_l10n::GetLocalizedResourceUTF16String(
      IDS_BRAVE_WAYBACK_MACHINE_INFOBAR_NOT_AVAILABLE_TEXT));
  views_visible_after_checking_.push_back(label);
  label->SetProperty(views::kFlexBehaviorKey, label_flex_rule);
  label->SetProperty(
      views::kMarginsKey,
      gfx::Insets::VH(ChromeLayoutProvider::Get()->GetDistanceMetric(
                          DISTANCE_TOAST_LABEL_VERTICAL),
                      0));
  AddChildView(label);

  fetch_url_button_ =
      AddChildView(std::make_unique<BraveWaybackMachineInfoBarButtonContainer>(
          base::BindRepeating(
              &BraveWaybackMachineInfoBarContentsView::FetchURLButtonPressed,
              base::Unretained(this))));
  views_visible_before_checking_.push_back(fetch_url_button_);
  fetch_url_button_->SetProperty(
      views::kMarginsKey,
      gfx::Insets::VH(ChromeLayoutProvider::Get()->GetDistanceMetric(
                          DISTANCE_TOAST_CONTROL_VERTICAL),
                      ChromeLayoutProvider::Get()->GetDistanceMetric(
                          DISTANCE_RELATED_CONTROL_HORIZONTAL_SMALL)));

  UpdateChildrenVisibility(true);
}

views::Label* BraveWaybackMachineInfoBarContentsView::CreateLabel(
    const std::u16string& text) {
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
  const auto* color_provider = GetColorProvider();
  return color_provider ? color_provider->GetColor(id) : gfx::kPlaceholderColor;
}

void BraveWaybackMachineInfoBarContentsView::FetchWaybackURL() {
  fetch_url_button_->StartThrobber();
  wayback_machine_url_fetcher_.Fetch(contents_->GetVisibleURL());
  Layout();
}

void BraveWaybackMachineInfoBarContentsView::LoadURL(const GURL& url) {
  contents_->GetController().LoadURL(url,
                                     content::Referrer(),
                                     ui::PAGE_TRANSITION_LINK,
                                     std::string());
}

BEGIN_METADATA(BraveWaybackMachineInfoBarContentsView, views::View)
END_METADATA
