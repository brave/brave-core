/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/psst/psst_consent_tab_helper_delegate_impl.h"

PsstConsentTabHelperDelegateImpl::PsstConsentTabHelperDelegateImpl() = default;

PsstConsentTabHelperDelegateImpl::~PsstConsentTabHelperDelegateImpl() = default;

#if !defined(TOOLKIT_VIEWS)
void PsstConsentTabHelperDelegateImpl::ShowPsstConsentDialog(
    content::WebContents* contents,
    base::OnceClosure yes_cb,
    base::OnceClosure no_cb) {
  NOTIMPLEMENTED();
}
#endif
