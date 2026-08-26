// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_PSST_PSST_INFOBAR_DELEGATE_H_
#define BRAVE_BROWSER_PSST_PSST_INFOBAR_DELEGATE_H_

#include "base/functional/callback.h"
#include "components/infobars/core/confirm_infobar_delegate.h"

namespace psst {

class PsstInfoBarDelegate : public ConfirmInfoBarDelegate {
 public:
  using AcceptCallback = base::OnceCallback<void(const bool is_accepted)>;

  PsstInfoBarDelegate(const PsstInfoBarDelegate&) = delete;
  PsstInfoBarDelegate& operator=(const PsstInfoBarDelegate&) = delete;

  ~PsstInfoBarDelegate() override;

  static void Create(infobars::InfoBarManager* infobar_manager,
                     AcceptCallback on_accept_callback);

  // ConfirmInfoBarDelegate overrides:

  // Handles infobar acceptance. The return value determines the upstream
  // behavior; return `true` if the infobar should be removed automatically.
  bool Accept() override;
  // Handles infobar cancellation. The return value determines upstream
  // behavior: return `true` if the infobar should be removed automatically.
  bool Cancel() override;
  // Handles infobar closing.
  void InfoBarDismissed() override;

  void DisableCallback();

 private:
  explicit PsstInfoBarDelegate(AcceptCallback on_accept_callback);

  // ConfirmInfoBarDelegate overrides:
  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override;
  std::u16string GetMessageText() const override;
  int GetButtons() const override;
  std::u16string GetButtonLabel(InfoBarButton button) const override;

  AcceptCallback on_accept_callback_;
};

}  // namespace psst

#endif  // BRAVE_BROWSER_PSST_PSST_INFOBAR_DELEGATE_H_
