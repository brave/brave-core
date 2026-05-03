// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/brave_actions/brave_shields_action_controller.h"

#include <string>
#include <utility>

#include "base/check_deref.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string_number_conversions.h"
#include "brave/browser/ui/views/brave_actions/brave_icon_with_badge_image_source.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/constants/url_constants.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/speedreader/common/buildflags/buildflags.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/bubble/webui_bubble_manager.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/url_constants.h"
#include "extensions/common/constants.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/image_model.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/color/color_provider_manager.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/image/image_skia_rep.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/view.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_SPEEDREADER)
#include "brave/browser/ui/speedreader/speedreader_tab_helper.h"
#endif

namespace {
constexpr SkColor kBadgeBg = SkColorSetRGB(0x63, 0x64, 0x72);
}  // namespace

BraveShieldsActionController::BraveShieldsActionController(
    BrowserWindowInterface* browser_window_interface,
    CreateWebUIBubbleManagerCallback create_bubble_manager_callback)
    : browser_window_interface_(browser_window_interface),
      create_bubble_manager_callback_(create_bubble_manager_callback),
      profile_(CHECK_DEREF(browser_window_interface->GetProfile())),
      tab_strip_model_(
          CHECK_DEREF(browser_window_interface->GetTabStripModel())) {
  if (content::WebContents* contents =
          tab_strip_model_->GetActiveWebContents()) {
    AddObserverToWebContentsIfPresent(contents);
  }
  tab_strip_model_->AddObserver(this);
}

BraveShieldsActionController::~BraveShieldsActionController() {
  RemoveObserverFromWebContents(tab_strip_model_->GetActiveWebContents());
}

void BraveShieldsActionController::SetAnchorView(views::View* anchor) {
  anchor_view_ = anchor;
}

void BraveShieldsActionController::SetOnStateChanged(
    base::RepeatingClosure callback) {
  on_state_changed_ = std::move(callback);
}

void BraveShieldsActionController::NotifyStateChanged() {
  if (on_state_changed_) {
    on_state_changed_.Run();
  }
}

int BraveShieldsActionController::IconDimensionForLayout() const {
  if (icon_style_ == IconStyle::kWebAppTitleBar) {
    return GetLayoutConstant(LayoutConstant::kWebAppPageActionIconSize);
  }
  return GetLayoutConstant(LayoutConstant::kLocationBarTrailingIconSize);
}

void BraveShieldsActionController::AddObserverToWebContentsIfPresent(
    content::WebContents* contents) {
  if (!contents) {
    return;
  }
  if (auto* tab_helper =
          brave_shields::BraveShieldsTabHelper::FromWebContents(contents)) {
    if (!tab_helper->HasObserver(this)) {
      tab_helper->AddObserver(this);
    }
  }
}

void BraveShieldsActionController::RemoveObserverFromWebContents(
    content::WebContents* contents) {
  if (!contents) {
    return;
  }
  if (auto* tab_helper =
          brave_shields::BraveShieldsTabHelper::FromWebContents(contents)) {
    if (tab_helper->HasObserver(this)) {
      tab_helper->RemoveObserver(this);
    }
  }
}

gfx::ImageSkia BraveShieldsActionController::GetIconImage(
    bool is_enabled) const {
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  gfx::ImageSkia image;
  const SkBitmap bitmap =
      rb.GetImageNamed(is_enabled ? IDR_BRAVE_SHIELDS_ICON_64
                                  : IDR_BRAVE_SHIELDS_ICON_64_DISABLED)
          .AsBitmap();
  const int icon_size = IconDimensionForLayout();
  float scale = static_cast<float>(bitmap.width()) / icon_size;
  image.AddRepresentation(gfx::ImageSkiaRep(bitmap, scale));
  return image;
}

std::unique_ptr<IconWithBadgeImageSource>
BraveShieldsActionController::GetImageSource(
    const gfx::Size& preferred_size) const {
  auto* web_contents = tab_strip_model_->GetActiveWebContents();

  auto get_color_provider_callback = base::BindRepeating(
      [](base::WeakPtr<content::WebContents> weak_web_contents) {
        const auto* const color_provider =
            weak_web_contents
                ? &weak_web_contents->GetColorProvider()
                : ui::ColorProviderManager::Get().GetColorProviderFor(
                      ui::NativeTheme::GetInstanceForNativeUi()
                          ->GetColorProviderKey(nullptr));
        return color_provider;
      },
      web_contents ? web_contents->GetWeakPtr()
                   : base::WeakPtr<content::WebContents>());

  std::unique_ptr<IconWithBadgeImageSource> image_source(
      new brave::BraveIconWithBadgeImageSource(
          preferred_size, std::move(get_color_provider_callback),
          IconDimensionForLayout(), kBraveActionLeftMarginExtra));
  std::unique_ptr<IconWithBadgeImageSource::Badge> badge;
  bool is_enabled = false;
  std::string badge_text;

  if (web_contents) {
    auto* shields_data_controller =
        brave_shields::BraveShieldsTabHelper::FromWebContents(web_contents);

    int count = shields_data_controller->GetTotalBlockedCount();
    if (count > 0) {
      badge_text = count > 99 ? "99+" : base::NumberToString(count);
    }

    is_enabled = shields_data_controller->GetBraveShieldsEnabled() &&
                 !IsPageInReaderMode(web_contents);

    if (!badge_text.empty()) {
      badge = std::make_unique<IconWithBadgeImageSource::Badge>(
          badge_text, SK_ColorWHITE, kBadgeBg);
    }
  }

  image_source->SetIcon(gfx::Image(GetIconImage(is_enabled)));

  if (is_enabled &&
      profile_->GetPrefs()->GetBoolean(kShieldsStatsBadgeVisible)) {
    image_source->SetBadge(std::move(badge));
  }

  return image_source;
}

ui::ImageModel BraveShieldsActionController::GetImageModel(
    const gfx::Size& preferred_size) const {
  return ui::ImageModel::FromImageSkia(
      gfx::ImageSkia(GetImageSource(preferred_size), preferred_size));
}

void BraveShieldsActionController::RefreshButtonImages(
    views::LabelButton* button) {
  if (!button) {
    return;
  }
  const gfx::Size preferred = button->GetPreferredSize();
  button->SetImageModel(views::Button::STATE_NORMAL, GetImageModel(preferred));
}

void BraveShieldsActionController::OnButtonPressed() {
  auto* web_content = tab_strip_model_->GetActiveWebContents();
  if (!ShouldShowBubbleForContents(web_content)) {
    return;
  }

  if (webui_bubble_manager_ && webui_bubble_manager_->GetBubbleWidget()) {
    webui_bubble_manager_->CloseBubble();
    return;
  }

  ShowBubble(GURL(kShieldsPanelURL));
}

bool BraveShieldsActionController::IsPageInReaderMode(
    content::WebContents* web_contents) const {
  if (!web_contents) {
    return false;
  }
#if BUILDFLAG(ENABLE_SPEEDREADER)
  if (auto* speedreader_tab_helper =
          speedreader::SpeedreaderTabHelper::FromWebContents(web_contents)) {
    return speedreader::DistillStates::IsDistilled(
        speedreader_tab_helper->PageDistillState());
  }
#endif
  return false;
}

bool BraveShieldsActionController::ShouldShowBubbleForContents(
    content::WebContents* web_contents) const {
  if (!web_contents) {
    return false;
  }
  const GURL& url = web_contents->GetLastCommittedURL();

  if (url.SchemeIs(url::kAboutScheme) || url.SchemeIs(url::kBlobScheme) ||
      url.SchemeIs(url::kDataScheme) || url.SchemeIs(url::kFileSystemScheme) ||
      url.SchemeIs(kMagnetScheme) || url.SchemeIs(content::kChromeUIScheme) ||
      url.SchemeIs(extensions::kExtensionScheme)) {
    // Do not show bubble if it is a local scheme.
    return false;
  }

  if (IsPageInReaderMode(web_contents)) {
    // Do not show bubble on speedreader pages.
    return false;
  }

  return true;
}

void BraveShieldsActionController::ShowBubble(GURL webui_url) {
  if (!anchor_view_) {
    return;
  }
  if (!webui_bubble_manager_ || webui_url != last_webui_url_) {
    webui_bubble_manager_ = create_bubble_manager_callback_.Run(
        anchor_view_, browser_window_interface_, webui_url, IDS_BRAVE_SHIELDS,
        /*force_load_on_create=*/false);
  }
  last_webui_url_ = webui_url;

  webui_bubble_manager_->ShowBubble();
}

views::Widget* BraveShieldsActionController::GetBubbleWidget() {
  if (!webui_bubble_manager_) {
    return nullptr;
  }
  return webui_bubble_manager_->GetBubbleWidget();
}

std::u16string BraveShieldsActionController::GetTooltipText() const {
  if (content::WebContents* web_contents =
          tab_strip_model_->GetActiveWebContents()) {
    if (auto* shields_data_controller =
            brave_shields::BraveShieldsTabHelper::FromWebContents(
                web_contents)) {
      int count = shields_data_controller->GetTotalBlockedCount();
      if (count > 0) {
        return l10n_util::GetStringFUTF16Int(IDS_BRAVE_SHIELDS_ICON_TOOLTIP,
                                             count);
      }
    }
  }
  return l10n_util::GetStringUTF16(IDS_BRAVE_SHIELDS);
}

void BraveShieldsActionController::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  if (selection.active_tab_changed()) {
    if (selection.old_contents) {
      RemoveObserverFromWebContents(selection.old_contents);
    }
    if (selection.new_contents) {
      AddObserverToWebContentsIfPresent(selection.new_contents);
    }
    NotifyStateChanged();
  }
}

void BraveShieldsActionController::OnResourcesChanged() {
  NotifyStateChanged();
}

void BraveShieldsActionController::OnShieldsEnabledChanged() {
  NotifyStateChanged();
}

void BraveShieldsActionController::OnRepeatedReloadsDetected() {
  auto* web_content = tab_strip_model_->GetActiveWebContents();
  if (!ShouldShowBubbleForContents(web_content)) {
    return;
  }

  ShowBubble(
      GURL(std::string(kShieldsPanelURL) + "?mode=afterRepeatedReloads"));
}
