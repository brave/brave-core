/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PSST_PSST_CONSENT_TAB_HELPER_DELEGATE_IMPL_H_
#define BRAVE_BROWSER_PSST_PSST_CONSENT_TAB_HELPER_DELEGATE_IMPL_H_

#include <memory>
#include <string>
#include "base/values.h"

#include "brave/browser/ui/views/psst/psst_consent_dialog.h"
#include "brave/components/psst/browser/content/psst_tab_helper.h"

class PsstConsentTabHelperDelegateImpl : public psst::PsstTabHelper::Delegate {
 public:
  PsstConsentTabHelperDelegateImpl();
  ~PsstConsentTabHelperDelegateImpl() override;

  // psst::PsstTabHelper::Delegate
  void ShowPsstConsentDialog(content::WebContents* contents,
                             bool prompt_for_new_version,
                             base::Value::List requests,
                             base::OnceClosure yes_cb,
                             base::OnceClosure no_cb) override;
  void SetProgressValue(content::WebContents* contents, const double value) override;
  void SetRequestDone(content::WebContents* contents, const std::string& url) override;
  void SetRequestError(content::WebContents* contents, const std::string& url, const std::string& error) override;
  void Close(content::WebContents* contents) override;
// private:
//     raw_ptr<PsstConsentDialog> psst_content_dialog_{nullptr};
};

#endif  // BRAVE_BROWSER_PSST_PSST_CONSENT_TAB_HELPER_DELEGATE_IMPL_H_
