/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/psst/psst_consent_tab_helper_delegate_impl.h"

#include "base/functional/callback.h"
#include "brave/browser/ui/views/psst/psst_consent_dialog.h"
#include "components/constrained_window/constrained_window_views.h"
#include "content/public/browser/web_contents.h"

void PsstConsentTabHelperDelegateImpl::ShowPsstConsentDialog(
    content::WebContents* contents,
    base::OnceClosure yes_cb,
    base::OnceClosure no_cb) {
  constrained_window::ShowWebModalDialogViews(
      new PsstConsentDialog(std::move(yes_cb), std::move(no_cb)), contents);
}
