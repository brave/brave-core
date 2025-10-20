/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_browser_widget.h"

#include "brave/browser/themes/brave_private_window_theme_supplier.h"
#include "brave/browser/ui/darker_theme/features.h"
#include "brave/browser/ui/darker_theme/pref_names.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/tabs/shared_pinned_tab_service.h"
#include "brave/browser/ui/tabs/shared_pinned_tab_service_factory.h"
#include "brave/browser/ui/views/frame/brave_browser_root_view.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/browser_widget.h"
#include "components/prefs/pref_service.h"
#include "ui/color/color_provider_key.h"

BraveBrowserWidget::BraveBrowserWidget(BrowserView* browser_view)
    : BrowserWidget(browser_view), view_(browser_view) {
  if (view_->browser()->profile()->IsIncognitoProfile() ||
      view_->browser()->profile()->IsTor() ||
      view_->browser()->profile()->IsGuestSession()) {
    theme_supplier_ = base::MakeRefCounted<BravePrivateWindowThemeSupplier>(
        !view_->browser()->profile()->IsTor());
  }
}

BraveBrowserWidget::~BraveBrowserWidget() {
  // Some modules need to get fullscreen state change but they can't
  // know the observing stop timing by themselves.
  // As exclusive_access_manager() is destroyed from BrowserWindowFeatures
  // at the start of BrowserWidget dtor, this method is should be called here.
  BraveBrowserView::From(view_)->StopListeningFullscreenChanges();
}

ui::ColorProviderKey::ThemeInitializerSupplier*
BraveBrowserWidget::GetCustomTheme() const {
  // To provider private(tor) windows's theme color via color provider,
  // we use |theme_supplier_| for both as upstream doesn't use separated
  // mix for private window.
  if (theme_supplier_) {
    return theme_supplier_.get();
  }

  return BrowserWidget::GetCustomTheme();
}

views::internal::RootView* BraveBrowserWidget::CreateRootView() {
  root_view_ = new BraveBrowserRootView(browser_view_, this);
  return root_view_;
}

void BraveBrowserWidget::SetTabDragKind(TabDragKind kind) {
  const bool should_sync_shared_pinned_tab =
      base::FeatureList::IsEnabled(tabs::features::kBraveSharedPinnedTabs) &&
      tab_drag_kind_ == TabDragKind::kAllTabs && kind == TabDragKind::kNone;

  BrowserWidget::SetTabDragKind(kind);

  if (should_sync_shared_pinned_tab) {
    SharedPinnedTabServiceFactory::GetForProfile(browser_view_->GetProfile())
        ->TabDraggingEnded(browser_view_->browser());
  }
}

ui::ColorProviderKey BraveBrowserWidget::GetColorProviderKey() const {
  auto key = BrowserWidget::GetColorProviderKey();

  // We want to use dark mode for guest profile.
  if (view_->browser()->profile()->IsGuestSession()) {
    key.color_mode = ui::ColorProviderKey::ColorMode::kDark;
    key.user_color_source = ui::ColorProviderKey::UserColorSource::kGrayscale;
  }

  if (base::FeatureList::IsEnabled(darker_theme::features::kBraveDarkerTheme)) {
    // Note that we don't change set SchemeVariant to kDarker if
    // |theme_supplier_| exists because |theme_supplier_| is for
    // private/tor/guest window and we don't want to set kDarker for them.
    if (!theme_supplier_ &&
        key.color_mode == ui::ColorProviderKey::ColorMode::kDark &&
        browser_view_->browser()->profile()->GetPrefs()->GetBoolean(
            darker_theme::prefs::kBraveDarkerMode)) {
      key.scheme_variant = ui::ColorProviderKey::SchemeVariant::kDarker;
    }
  }

  return key;
}
