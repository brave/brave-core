/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PSST_PSST_CONSENT_DIALOG_H_
#define BRAVE_BROWSER_UI_VIEWS_PSST_PSST_CONSENT_DIALOG_H_

#include <memory>
#include <string>

#include "base/containers/flat_map.h"
#include "base/functional/callback.h"
#include "base/functional/callback_forward.h"
#include "base/values.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/button/checkbox.h"
#include "ui/views/controls/progress_bar.h"
#include "ui/views/window/dialog_delegate.h"

class PsstConsentDialog : public views::DialogDelegateView {
 public:
 struct StatusCheckedLine {
  raw_ptr<views::Checkbox> check_box{nullptr};
  raw_ptr<views::Label> status_label{nullptr};
 };

  PsstConsentDialog(bool prompt_for_new_version,
                    base::Value::List requests,
                    base::OnceClosure consent_callback,
                    base::OnceClosure cancel_callback);
  ~PsstConsentDialog() override;

  // views::DialogDelegateView:
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void WindowClosing() override;

  void SetProgressValue(const double value);

  void SetRequestDone(const std::string& url);

  void SetRequestError(const std::string& url, const std::string& error);

  void OnConsentClicked();

 private:
  void DisableAdBlockForSite();

  base::OnceClosure consent_callback_;
  raw_ptr<views::Button> no_button_{nullptr};
  raw_ptr<views::Button> ok_button_{nullptr};
  raw_ptr<views::ProgressBar> progress_bar_{nullptr};
  base::flat_map<std::string, std::unique_ptr<StatusCheckedLine>> task_checked_list_;
  base::WeakPtrFactory<PsstConsentDialog> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PSST_PSST_CONSENT_DIALOG_H_
