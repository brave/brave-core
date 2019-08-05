/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_INFOBARS_BRAVE_CONFIRM_P3A_INFOBAR_DELEGATE_H_
#define BRAVE_BROWSER_INFOBARS_BRAVE_CONFIRM_P3A_INFOBAR_DELEGATE_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "components/infobars/core/confirm_infobar_delegate.h"
#include "url/gurl.h"

#include "brave/browser/infobars/brave_infobar_delegate.h"

class InfoBarService;
class PrefService;

// An infobar that is run with a string and a "Learn More" link.
class BraveConfirmP3AInfoBarDelegate : public ConfirmInfoBarDelegate {
 public:
  // Creates a missing Google API Keys infobar and delegate and adds the infobar
  // to |infobar_service|.
  static void Create(InfoBarService* infobar_service, PrefService* local_state);

 private:
  BraveConfirmP3AInfoBarDelegate(PrefService* local_state);
  ~BraveConfirmP3AInfoBarDelegate() override;

  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override;
  const gfx::VectorIcon& GetVectorIcon() const override;
  bool ShouldExpire(const NavigationDetails& details) const override;
  void InfoBarDismissed() override;
  base::string16 GetMessageText() const override;
  int GetButtons() const override;
  base::string16 GetButtonLabel(InfoBarButton button) const override;
  base::string16 GetLinkText() const override;
  GURL GetLinkURL() const override;
  bool Accept() override;
  bool Cancel() override;

  PrefService* local_state_;

  DISALLOW_COPY_AND_ASSIGN(BraveConfirmP3AInfoBarDelegate);
};

#endif  // BRAVE_BROWSER_INFOBARS_BRAVE_CONFIRM_P3A_INFOBAR_DELEGATE_H_
