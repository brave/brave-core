/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/translate/brave_translate_icon_view.h"

#include "chrome/browser/ui/browser.h"
#include "brave/common/extensions/extension_constants.h"
#include "chrome/browser/extensions/webstore_install_with_prompt.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/views/translate/translate_bubble_view.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/extension_registry.h"

BraveTranslateIconView::BraveTranslateIconView(
    CommandUpdater* command_updater,
    PageActionIconView::Delegate* delegate)
    : TranslateIconView(command_updater, delegate),
      weak_ptr_factory_(this) {
}

BraveTranslateIconView::~BraveTranslateIconView() {
}

void BraveTranslateIconView::InstallGoogleTranslate() {
  if (!GetWebContents())
    return;
  Browser* browser = chrome::FindBrowserWithWebContents(GetWebContents());
  if (!browser)
    return;

  scoped_refptr<extensions::WebstoreInstallWithPrompt> installer =
        new extensions::WebstoreInstallWithPrompt(
            google_translate_extension_id,
            Profile::FromBrowserContext(
              GetWebContents()->GetBrowserContext()),
            browser->window()->GetNativeWindow(),
            base::BindOnce(&BraveTranslateIconView::OnInstallResult,
              weak_ptr_factory_.GetWeakPtr()));
  installer->BeginInstall();
}

void BraveTranslateIconView::OnInstallResult(
    bool success,
    const std::string& error,
    extensions::webstore_install::Result result) {
  if (!success) return;
  Update();
}

void BraveTranslateIconView::UpdateImpl() {
  if (!GetWebContents())
    return;

  // Hide TranslateIcon & TranslateBubble when google translate extension is
  // already installed.
  extensions::ExtensionRegistry* registry =
    extensions::ExtensionRegistry::Get(GetWebContents()->GetBrowserContext());
  if (registry->GetInstalledExtension(google_translate_extension_id)) {
    SetVisible(false);
    TranslateBubbleView::CloseCurrentBubble();
  }

  TranslateIconView::UpdateImpl();
}
