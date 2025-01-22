/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PSST_PSST_CONSENT_DIALOG_H_
#define BRAVE_BROWSER_UI_VIEWS_PSST_PSST_CONSENT_DIALOG_H_

#include <string>
#include "base/values.h"

#include "base/functional/callback.h"
#include "base/functional/callback_forward.h"
#include "ui/views/window/dialog_delegate.h"

class PsstConsentDialog : public views::DialogDelegateView {
 public:
  PsstConsentDialog(bool prompt_for_new_version,
                    const std::string& list_of_changes,
                    base::OnceClosure yes_callback,
                    base::OnceClosure no_callback);
  ~PsstConsentDialog() override;

  // views::DialogDelegateView:
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void WindowClosing() override;

 private:
  void DisableAdBlockForSite();
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PSST_PSST_CONSENT_DIALOG_H_
