/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/psst/psst_consent_tab_helper_delegate_impl.h"
#include "base/values.h"

PsstConsentTabHelperDelegateImpl::PsstConsentTabHelperDelegateImpl() = default;

PsstConsentTabHelperDelegateImpl::~PsstConsentTabHelperDelegateImpl() = default;

#if !defined(TOOLKIT_VIEWS)
void PsstConsentTabHelperDelegateImpl::ShowPsstConsentDialog(
    content::WebContents* contents,
    bool prompt_for_new_version,
    const std::string& list_of_changes,
    base::OnceClosure yes_cb,
    base::OnceClosure no_cb) {
  NOTIMPLEMENTED();
}

void PsstConsentTabHelperDelegateImpl::SetProgressValue(content::WebContents* contents, const double value) override {
  NOTIMPLEMENTED();
}

void PsstConsentTabHelperDelegateImpl::Close(content::WebContents* contents) {
  NOTIMPLEMENTED();
}
#endif
