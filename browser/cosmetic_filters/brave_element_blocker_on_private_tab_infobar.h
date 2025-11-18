/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_COSMETIC_FILTERS_BRAVE_ELEMENT_BLOCKER_ON_PRIVATE_TAB_INFOBAR_H_
#define BRAVE_BROWSER_COSMETIC_FILTERS_BRAVE_ELEMENT_BLOCKER_ON_PRIVATE_TAB_INFOBAR_H_

#include "base/memory/raw_ptr.h"
#include "brave/components/infobars/core/brave_confirm_infobar_delegate.h"

namespace infobars {
class ContentInfoBarManager;
}  // namespace infobars

namespace content {
class WebContents;
}  // namespace content

class BraveElementBlockerOnPrivateTabInfoBarDelegate
    : public BraveConfirmInfoBarDelegate {
 public:
  BraveElementBlockerOnPrivateTabInfoBarDelegate(
      const BraveElementBlockerOnPrivateTabInfoBarDelegate&) = delete;
  BraveElementBlockerOnPrivateTabInfoBarDelegate& operator=(
      const BraveElementBlockerOnPrivateTabInfoBarDelegate&) = delete;

  static void Create(infobars::ContentInfoBarManager* infobar_manager,
                     content::WebContents* source_web_contents);

 private:
  explicit BraveElementBlockerOnPrivateTabInfoBarDelegate(
      content::WebContents* source_web_contents);
  ~BraveElementBlockerOnPrivateTabInfoBarDelegate() override;

  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override;
  const gfx::VectorIcon& GetVectorIcon() const override;
  bool ShouldExpire(const NavigationDetails& details) const override;
  void InfoBarDismissed() override;
  std::u16string GetMessageText() const override;
  int GetButtons() const override;
  //   std::u16string GetButtonLabel(InfoBarButton button) const override;
  //   std::u16string GetLinkText() const override;
  //   GURL GetLinkURL() const override;
  bool Accept() override;
  bool Cancel() override;

  raw_ptr<content::WebContents> source_web_contents_;
};

#endif  // BRAVE_BROWSER_COSMETIC_FILTERS_BRAVE_ELEMENT_BLOCKER_ON_PRIVATE_TAB_INFOBAR_H_
