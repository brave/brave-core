/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/infobars/brave_global_infobar_manager.h"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>
#include "brave/browser/ui/views/infobars/brave_confirm_infobar.h"
#include "brave/components/infobars/core/brave_confirm_infobar_delegate.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "components/infobars/core/infobar.h"
#include "components/infobars/core/infobar_manager.h"

namespace {
std::unique_ptr<infobars::InfoBar> CreateBraveGlobalInfoBar(
    std::unique_ptr<BraveConfirmInfoBarDelegate> delegate) {
  return std::make_unique<BraveConfirmInfoBar>(std::move(delegate));
}

content::WebContents* GetCurrentWebContents() {
  Browser* browser = chrome::FindLastActive();
  if (!browser) {
    return nullptr;
  }

  TabStripModel* model = browser->tab_strip_model();
  if (!model) {
    return nullptr;
  }

  return model->GetActiveWebContents();
}

infobars::InfoBarManager* GetCurrentInfoBarManager() {
  auto* web_contents = GetCurrentWebContents();
  if (!web_contents) {
    return nullptr;
  }

  infobars::ContentInfoBarManager* infobar_manager =
      infobars::ContentInfoBarManager::FromWebContents(web_contents);
  if (!infobar_manager) {
    return nullptr;
  }

  return infobar_manager;
}

void RemoveInfobarsByIdentifier(
    infobars::InfoBarManager* infobar_manager,
    const infobars::InfoBarDelegate::InfoBarIdentifier& id) {
  for (size_t i = 0; i < infobar_manager->infobar_count(); i++) {
    if (auto* current_infobar = infobar_manager->infobar_at(i);
        current_infobar->delegate()->GetIdentifier() == id) {
      infobar_manager->RemoveInfoBar(current_infobar);
    }
  }
}
}  // namespace

BraveGlobalInfoBarManager::BraveGlobalInfoBarManager(
    std::unique_ptr<BraveConfirmInfoBarDelegateFactory> delegate_factory)
    : delegate_factory_(std::move(delegate_factory)) {}

BraveGlobalInfoBarManager::~BraveGlobalInfoBarManager() {
  infobars::InfoBarManager* infobar_manager = GetCurrentInfoBarManager();
  if (!infobar_manager) {
    return;
  }
  infobar_manager->RemoveObserver(this);
  RemoveInfobarsByIdentifier(infobar_manager,
                             delegate_factory_->GetInfoBarIdentifier());
}

void BraveGlobalInfoBarManager::Show() {
  is_closed_ = false;
  browser_tab_strip_tracker_ =
      std::make_unique<BrowserTabStripTracker>(this, nullptr);
  browser_tab_strip_tracker_->Init();
}

void BraveGlobalInfoBarManager::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  if (!selection.new_contents || is_closed_) {
    return;
  }

  MaybeAddInfoBar(selection.new_contents);

  if (!selection.old_contents || !selection.active_tab_changed()) {
    return;
  }

  infobars::ContentInfoBarManager* old_infobar_manager =
      infobars::ContentInfoBarManager::FromWebContents(selection.old_contents);

  if (!old_infobar_manager) {
    return;
  }
  old_infobar_manager->RemoveObserver(this);
  RemoveInfobarsByIdentifier(old_infobar_manager,
                             delegate_factory_->GetInfoBarIdentifier());
}

void BraveGlobalInfoBarManager::MaybeAddInfoBar(
    content::WebContents* web_contents) {
  infobars::ContentInfoBarManager* infobar_manager =
      infobars::ContentInfoBarManager::FromWebContents(web_contents);
  DCHECK(infobar_manager);
  if (!infobar_manager) {
    return;
  }

  auto delegate = delegate_factory_->Create();
  if (!delegate) {
    return;
  }

  if (infobars::InfoBar const* added_bar = infobar_manager->AddInfoBar(
          CreateBraveGlobalInfoBar(std::move(delegate)));
      !added_bar) {
    return;
  }
  infobar_manager->RemoveObserver(this);
  infobar_manager->AddObserver(this);
}

void BraveGlobalInfoBarManager::OnInfoBarRemoved(infobars::InfoBar* info_bar,
                                                 bool animate) {
  infobars::InfoBarManager* current_infobar_manager =
      GetCurrentInfoBarManager();
  if (!current_infobar_manager) {
    return;
  }
  if (delegate_factory_->GetInfoBarIdentifier() !=
          info_bar->delegate()->GetIdentifier() ||
      current_infobar_manager != info_bar->owner()) {
    return;
  }
  OnManagerShuttingDown(info_bar->owner());
}

void BraveGlobalInfoBarManager::OnManagerShuttingDown(
    infobars::InfoBarManager* manager) {
  manager->RemoveObserver(this);
  is_closed_ = true;
}
