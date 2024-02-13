/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_GLOBAL_INFOBAR_MANAGER_H_
#define BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_GLOBAL_INFOBAR_MANAGER_H_

#include <memory>

#include "brave/browser/infobars/brave_global_confirm_infobar_delegate.h"
#include "chrome/browser/ui/browser_tab_strip_tracker.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"

namespace infobars {
class InfoBarManager;
}

class BraveGlobalInfoBarManager
    : public TabStripModelObserver,
      public BraveGlobalConfirmInfobarDelegate::Observer {
 public:
  explicit BraveGlobalInfoBarManager(
      std::unique_ptr<BraveGlobalConfirmInfoBarDelegateFactory>
          delegate_factory);
  ~BraveGlobalInfoBarManager() override;

  void Show();

 private:
  // TabStripModelObserver:
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;
  void TabChangedAt(content::WebContents* contents,
                    int index,
                    TabChangeType change_type) override;

  void MaybeAddInfoBar(infobars::InfoBarManager* infobar_manager);
  void OnInfoBarClosed() override;

  bool is_closed_{true};
  std::unique_ptr<BrowserTabStripTracker> browser_tab_strip_tracker_;
  std::unique_ptr<BraveGlobalConfirmInfoBarDelegateFactory> delegate_factory_;
  base::WeakPtrFactory<BraveGlobalInfoBarManager> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_GLOBAL_INFOBAR_MANAGER_H_
