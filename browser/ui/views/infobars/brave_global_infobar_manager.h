/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_GLOBAL_INFOBAR_MANAGER_H_
#define BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_GLOBAL_INFOBAR_MANAGER_H_

#include <memory>

#include "brave/components/infobars/core/brave_confirm_infobar_delegate.h"
#include "chrome/browser/ui/browser_tab_strip_tracker.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"

class BraveGlobalInfoBarManager : public TabStripModelObserver,
                                  public infobars::InfoBarManager::Observer {
 public:
  explicit BraveGlobalInfoBarManager(
      std::unique_ptr<BraveConfirmInfoBarDelegateFactory> delegate_factory);
  ~BraveGlobalInfoBarManager() override;

  void Show();

 private:
  // infobars::InfoBarManager::Observer:
  void OnInfoBarRemoved(infobars::InfoBar* info_bar, bool animate) override;
  void OnManagerShuttingDown(infobars::InfoBarManager* manager) override;

  // TabStripModelObserver:
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;

  void MaybeAddInfoBar(content::WebContents* web_contents);

  bool is_closed_{false};
  std::unique_ptr<BrowserTabStripTracker> browser_tab_strip_tracker_;
  std::unique_ptr<BraveConfirmInfoBarDelegateFactory> delegate_factory_;
  base::WeakPtrFactory<BraveGlobalInfoBarManager> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_GLOBAL_INFOBAR_MANAGER_H_
