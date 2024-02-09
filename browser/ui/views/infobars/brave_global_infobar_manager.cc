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
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "components/infobars/core/infobar.h"
#include "components/infobars/core/infobar_manager.h"
#include "content/public/browser/browser_context.h"

namespace {
std::unique_ptr<infobars::InfoBar> CreateBraveGlobalInfoBar(
    std::unique_ptr<BraveGlobalConfirmInfobarDelegate> delegate) {
  return std::make_unique<BraveConfirmInfoBar>(std::move(delegate));
}

void RemoveInfobarsByIdentifier(
    infobars::InfoBarManager* infobar_manager,
    const infobars::InfoBarDelegate::InfoBarIdentifier& id,
    BraveGlobalInfoBarManager* observer) {
  for (infobars::InfoBar* current_infobar : infobar_manager->infobars()) {
    if (current_infobar->delegate()->GetIdentifier() == id) {
      static_cast<BraveGlobalConfirmInfobarDelegate*>(
          current_infobar->delegate())
          ->RemoveObserver(observer);
      infobar_manager->RemoveInfoBar(current_infobar);
    }
  }
}

void RemoveAllInfobarsByIdentifier(
    const infobars::InfoBarDelegate::InfoBarIdentifier& id,
    BraveGlobalInfoBarManager* observer) {
  for (const Browser* browser : *BrowserList::GetInstance()) {
    const auto* tab_strip_model = browser->tab_strip_model();
    for (int i = 0; i < tab_strip_model->count(); ++i) {
      auto* web_contents = tab_strip_model->GetWebContentsAt(i);
      DCHECK(web_contents);
      if (!web_contents) {
        continue;
      }
      if (auto* infobar_manager =
              infobars::ContentInfoBarManager::FromWebContents(web_contents)) {
        RemoveInfobarsByIdentifier(infobar_manager, id, observer);
      }
    }
  }
}
}  // namespace

BraveGlobalInfoBarManager::BraveGlobalInfoBarManager(
    std::unique_ptr<BraveGlobalConfirmInfoBarDelegateFactory> delegate_factory)
    : delegate_factory_(std::move(delegate_factory)) {}

BraveGlobalInfoBarManager::~BraveGlobalInfoBarManager() {
  RemoveAllInfobarsByIdentifier(delegate_factory_->GetInfoBarIdentifier(),
                                this);
}

void BraveGlobalInfoBarManager::Show() {
  DCHECK(is_closed_);
  if (!is_closed_) {
    return;
  }
  is_closed_ = false;
  browser_tab_strip_tracker_ =
      std::make_unique<BrowserTabStripTracker>(this, nullptr);
  browser_tab_strip_tracker_->Init();
}

void BraveGlobalInfoBarManager::TabChangedAt(content::WebContents* contents,
                                             int index,
                                             TabChangeType change_type) {
  if (contents && contents->GetBrowserContext()->IsOffTheRecord()) {
    return;
  }

  infobars::ContentInfoBarManager* infobar_manager =
      infobars::ContentInfoBarManager::FromWebContents(contents);
  MaybeAddInfoBar(infobar_manager);
}

void BraveGlobalInfoBarManager::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  if (!selection.new_contents || is_closed_ ||
      selection.new_contents->GetBrowserContext()->IsOffTheRecord()) {
    return;
  }

  infobars::ContentInfoBarManager* new_infobar_manager =
      infobars::ContentInfoBarManager::FromWebContents(selection.new_contents);
  if (!new_infobar_manager) {
    return;
  }

  MaybeAddInfoBar(new_infobar_manager);

  if (!selection.old_contents || !selection.active_tab_changed()) {
    return;
  }

  infobars::ContentInfoBarManager* old_infobar_manager =
      infobars::ContentInfoBarManager::FromWebContents(selection.old_contents);

  if (!old_infobar_manager) {
    return;
  }
  RemoveInfobarsByIdentifier(old_infobar_manager,
                             delegate_factory_->GetInfoBarIdentifier(), this);
}

void BraveGlobalInfoBarManager::MaybeAddInfoBar(
    infobars::InfoBarManager* infobar_manager) {
  DCHECK(infobar_manager);
  if (!infobar_manager) {
    return;
  }
  auto delegate = delegate_factory_->Create();
  if (!delegate) {
    is_closed_ = true;
    return;
  }

  delegate->AddObserver(this);

  if (infobars::InfoBar const* added_bar = infobar_manager->AddInfoBar(
          CreateBraveGlobalInfoBar(std::move(delegate)));
      !added_bar) {
    return;
  }
}

void BraveGlobalInfoBarManager::OnInfoBarClosed() {
  RemoveAllInfobarsByIdentifier(delegate_factory_->GetInfoBarIdentifier(),
                                this);
  is_closed_ = true;
  browser_tab_strip_tracker_.reset();
}
