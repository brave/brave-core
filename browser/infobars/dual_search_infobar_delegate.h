/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_INFOBARS_DUAL_SEARCH_INFOBAR_DELEGATE_H_
#define BRAVE_BROWSER_INFOBARS_DUAL_SEARCH_INFOBAR_DELEGATE_H_

#include <string>

#include "brave/components/infobars/core/brave_confirm_infobar_delegate.h"

class PrefService;

namespace content {
class WebContents;
}  // namespace content

namespace infobars {
class ContentInfoBarManager;
}  // namespace infobars

// Infobar to inform users about the dual search split view feature
class DualSearchInfoBarDelegate : public BraveConfirmInfoBarDelegate {
 public:
  static void Create(infobars::ContentInfoBarManager* infobar_manager,
                     PrefService* prefs,
                     content::WebContents* web_contents);

  DualSearchInfoBarDelegate(const DualSearchInfoBarDelegate&) = delete;
  DualSearchInfoBarDelegate& operator=(const DualSearchInfoBarDelegate&) =
      delete;

 private:
  explicit DualSearchInfoBarDelegate(PrefService* prefs,
                                     content::WebContents* web_contents);
  ~DualSearchInfoBarDelegate() override;

  // BraveConfirmInfoBarDelegate overrides:
  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override;
  ui::ImageModel GetIcon() const override;
  std::u16string GetMessageText() const override;
  int GetButtons() const override;
  std::u16string GetButtonLabel(InfoBarButton button) const override;
  bool Accept() override;
  bool Cancel() override;
  void InfoBarDismissed() override;
  bool ShouldSupportMultiLine() const override;
  bool ExtraButtonPressed() override;
  std::vector<int> GetButtonsOrder() const override;

  void CloseOtherTab(bool close_default_search);

  raw_ptr<PrefService> prefs_;
  raw_ptr<content::WebContents> web_contents_;
};

#endif  // BRAVE_BROWSER_INFOBARS_DUAL_SEARCH_INFOBAR_DELEGATE_H_
