/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/infobars/brave_global_infobar_manager.h"
#include <memory>
#include <utility>
#include "base/containers/contains.h"
#include "brave/browser/ui/views/infobars/brave_confirm_infobar.h"
#include "brave/components/infobars/core/brave_confirm_infobar_delegate.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "components/infobars/core/infobar.h"

std::unique_ptr<infobars::InfoBar> CreateBraveGlobalInfoBarManager(
    std::unique_ptr<BraveConfirmInfoBarDelegate> delegate) {
  return std::make_unique<BraveConfirmInfoBar>(std::move(delegate));
}

BraveGlobalInfoBarManager::BraveGlobalInfoBarManager(
    std::unique_ptr<BraveConfirmInfoBarDelegateFactory> delegate_factory)
    : delegate_factory_(std::move(delegate_factory)) {
  browser_tab_strip_tracker_.Init();
}
BraveGlobalInfoBarManager::~BraveGlobalInfoBarManager() {
  while (!delegates_.empty()) {
    auto it = delegates_.begin();
    it->first->RemoveObserver(this);
    it->first->RemoveInfoBar(it->second->GetInfobar());
    delegates_.erase(it);
  }
}

BraveGlobalInfoBarManager* BraveGlobalInfoBarManager::Show(
    std::unique_ptr<BraveConfirmInfoBarDelegateFactory> delegate_factory) {
  return new BraveGlobalInfoBarManager(std::move(delegate_factory));
}

void BraveGlobalInfoBarManager::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  if (!selection.new_contents) {
    return;
  }

  MaybeAddInfoBar(selection.new_contents);

  if (!selection.old_contents) {
    return;
  }

  infobars::ContentInfoBarManager* infobar_manager =
      infobars::ContentInfoBarManager::FromWebContents(selection.old_contents);

  if (!selection.selection_changed() || !infobar_manager ||
      !base::Contains(delegates_, infobar_manager)) {
    return;
  }

  auto* infobar_to_close = delegates_[infobar_manager]->GetInfobar();
  infobar_manager->RemoveObserver(this);
  infobar_manager->RemoveInfoBar(infobar_to_close);
  delegates_.erase(infobar_manager);
}

void BraveGlobalInfoBarManager::MaybeAddInfoBar(
    content::WebContents* web_contents) {
  infobars::ContentInfoBarManager* infobar_manager =
      infobars::ContentInfoBarManager::FromWebContents(web_contents);
  DCHECK(infobar_manager);
  if (base::Contains(delegates_, infobar_manager)) {
    return;
  }

  auto delegate = delegate_factory_->Create();
  auto* delegate_ptr = delegate.get();
  if (infobars::InfoBar const* added_bar = infobar_manager->AddInfoBar(
          CreateBraveGlobalInfoBarManager(std::move(delegate)));
      !added_bar) {
    return;
  }

  infobar_manager->AddObserver(this);
  delegates_[infobar_manager] = delegate_ptr;
}

void BraveGlobalInfoBarManager::OnInfoBarRemoved(infobars::InfoBar* info_bar,
                                                 bool animate) {
  OnManagerShuttingDown(info_bar->owner());
}

void BraveGlobalInfoBarManager::OnManagerShuttingDown(
    infobars::InfoBarManager* manager) {
  manager->RemoveObserver(this);
  delegates_.erase(manager);
  if (delegates_.empty()) {
    Close();
  }
}

void BraveGlobalInfoBarManager::Close() {
  delete this;
}
