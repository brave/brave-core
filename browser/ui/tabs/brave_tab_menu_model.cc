/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/brave_tab_menu_model.h"

#include "brave/browser/ui/tabs/tab_menu_model_delegate_proxy.h"
#include "chrome/browser/sessions/tab_restore_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model_delegate.h"
#include "chrome/grit/generated_resources.h"
#include "components/sessions/core/tab_restore_service.h"

BraveTabMenuModel::BraveTabMenuModel(ui::SimpleMenuModel::Delegate* delegate,
                                     TabStripModel* tab_strip_model,
                                     int index)
    : TabMenuModel(new TabMenuModelDelegateProxy(delegate,
                                                 tab_strip_model,
                                                 index),
                   tab_strip_model,
                   index),
      index_(index),
      tab_strip_model_(tab_strip_model) {
  // Take owner ship of TabMenuModelDelegateProxy instance.
  // There is no way to instantiate it before calling ctor of base class.
  delegate_proxy_.reset(TabMenuModel::delegate());
  Build();
}

BraveTabMenuModel::~BraveTabMenuModel() {
}

int BraveTabMenuModel::GetRestoreTabCommandStringId() const {
  int id = IDS_RESTORE_TAB;

  content::WebContents* tab = tab_strip_model_->GetWebContentsAt(index_);
  if (!tab)
    return id;

  auto* browser = chrome::FindBrowserWithWebContents(tab);
  auto* restore_service =
      TabRestoreServiceFactory::GetForProfile(browser->profile());
  if (!restore_service)
    return id;

  if (!restore_service->IsLoaded() || restore_service->entries().empty())
    return id;

  if (restore_service->entries().front()->type ==
          sessions::TabRestoreService::WINDOW)
      id = IDS_RESTORE_WINDOW;

  return id;
}

void BraveTabMenuModel::Build() {
  AddItemWithStringId(CommandCloseOtherTabs, IDS_TAB_CXMENU_CLOSEOTHERTABS);
  AddSeparator(ui::NORMAL_SEPARATOR);
  AddItemWithStringId(CommandRestoreTab, GetRestoreTabCommandStringId());
  AddItemWithStringId(CommandBookmarkAllTabs, IDS_TAB_CXMENU_BOOKMARK_ALL_TABS);
}
