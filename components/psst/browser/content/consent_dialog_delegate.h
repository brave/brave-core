/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PSST_BROWSER_CONTENT_CONSENT_DIALOG_DELEGATE_H_
#define BRAVE_COMPONENTS_PSST_BROWSER_CONTENT_CONSENT_DIALOG_DELEGATE_H_

namespace psst {

class ConsentDialogDelegate {
public:
    virtual ~ConsentDialogDelegate() = default;
    
    virtual void ShowPsstConsentDialog() = 0;
    virtual void SetProgressValue() = 0;
    virtual void Close() = 0;
};

}  // namespace psst

#endif // BRAVE_COMPONENTS_PSST_BROWSER_CONTENT_CONSENT_DIALOG_DELEGATE_H_