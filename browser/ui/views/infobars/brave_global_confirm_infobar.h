/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_GLOBAL_CONFIRM_INFOBAR_H_
#define BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_GLOBAL_CONFIRM_INFOBAR_H_

#include "brave/browser/ui/views/infobars/brave_confirm_infobar.h"
#include "chrome/browser/ui/browser_tab_strip_tracker.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"

class BraveGlobalConfirmInfoBar : public TabStripModelObserver,
                                            public infobars::InfoBarManager::Observer {
  public:
    explicit BraveGlobalConfirmInfoBar(std::unique_ptr<BraveConfirmInfoBarDelegate> delegate);
    ~BraveGlobalConfirmInfoBar() override;

    static BraveGlobalConfirmInfoBar* Show(std::unique_ptr<BraveConfirmInfoBarDelegate> delegate);

  private:
  class DelegateProxy;

  void Close();

  // infobars::InfoBarManager::Observer:
  void OnInfoBarRemoved(infobars::InfoBar* info_bar, bool animate) override;
  void OnManagerShuttingDown(infobars::InfoBarManager* manager) override;

  // TabStripModelObserver:
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;

  void MaybeAddInfoBar(content::WebContents* web_contents);

  std::map<infobars::InfoBarManager*, DelegateProxy*> proxies_;
  BrowserTabStripTracker browser_tab_strip_tracker_{this, nullptr};
  std::unique_ptr<BraveConfirmInfoBarDelegate> delegate_;
  base::WeakPtrFactory<BraveGlobalConfirmInfoBar> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_GLOBAL_CONFIRM_INFOBAR_H_