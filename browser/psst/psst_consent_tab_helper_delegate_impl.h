/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PSST_PSST_CONSENT_TAB_HELPER_DELEGATE_IMPL_H_
#define BRAVE_BROWSER_PSST_PSST_CONSENT_TAB_HELPER_DELEGATE_IMPL_H_

#include <string>
#include "base/values.h"

#include "brave/components/psst/browser/content/psst_tab_helper.h"

class PsstConsentTabHelperDelegateImpl : public psst::PsstTabHelper::Delegate {
 public:
  PsstConsentTabHelperDelegateImpl();
  ~PsstConsentTabHelperDelegateImpl() override;

  // psst::PsstTabHelper::Delegate
  void ShowPsstConsentDialog(content::WebContents* contents,
                             bool prompt_for_new_version,
                             const std::string& list_of_changes,
                             base::OnceClosure yes_cb,
                             base::OnceClosure no_cb) override;
};

#endif  // BRAVE_BROWSER_PSST_PSST_CONSENT_TAB_HELPER_DELEGATE_IMPL_H_
