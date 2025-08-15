// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_PSST_BRAVE_PSST_INFOBAR_DELEGATE_H_
#define BRAVE_BROWSER_PSST_BRAVE_PSST_INFOBAR_DELEGATE_H_

#include "base/functional/callback.h"
#include "base/functional/callback_forward.h"
#include "components/infobars/core/confirm_infobar_delegate.h"

namespace psst {

class BravePsstInfoBarDelegate : public ConfirmInfoBarDelegate {
 public:
  using AcceptCallback = base::OnceCallback<void(const bool is_accepted)>;
  static void Create(infobars::InfoBarManager* infobar_manager,
                     AcceptCallback on_accept_callback);

  BravePsstInfoBarDelegate(const BravePsstInfoBarDelegate&) = delete;
  BravePsstInfoBarDelegate& operator=(const BravePsstInfoBarDelegate&) = delete;

  ~BravePsstInfoBarDelegate() override;

  bool Accept() override;
  bool Cancel() override;

 private:
  explicit BravePsstInfoBarDelegate(AcceptCallback on_accept_callback);

  // BraveConfirmInfoBarDelegate overrides:
  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override;
  std::u16string GetMessageText() const override;
  int GetButtons() const override;
  std::u16string GetButtonLabel(InfoBarButton button) const override;

  AcceptCallback on_accept_callback_;
};

}  // namespace psst

#endif  // BRAVE_BROWSER_PSST_BRAVE_PSST_INFOBAR_DELEGATE_H_
