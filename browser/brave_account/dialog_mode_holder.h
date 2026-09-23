/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ACCOUNT_DIALOG_MODE_HOLDER_H_
#define BRAVE_BROWSER_BRAVE_ACCOUNT_DIALOG_MODE_HOLDER_H_

#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class WebContents;
}  // namespace content

namespace brave_account {

// WebContentsUserData that holds onto the dialog mode the brave://account WebUI
// page was opened in. Set by the host surface that opens the page, before the
// page is loaded; read back when the page asks via
// `mojom::DialogController::GetDialogMode()`.
class DialogModeHolder : public content::WebContentsUserData<DialogModeHolder> {
 public:
  DialogModeHolder(const DialogModeHolder&) = delete;
  DialogModeHolder& operator=(const DialogModeHolder&) = delete;

  ~DialogModeHolder() override;

  // Replaces any dialog mode already set on `web_contents`.
  static void SetDialogMode(content::WebContents& web_contents,
                            mojom::DialogMode dialog_mode);

  // Returns `kDefault` if no dialog mode was set.
  static mojom::DialogMode GetDialogMode(content::WebContents& web_contents);

 private:
  friend class content::WebContentsUserData<DialogModeHolder>;

  DialogModeHolder(content::WebContents* web_contents,
                   mojom::DialogMode dialog_mode);

  const mojom::DialogMode dialog_mode_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace brave_account

#endif  // BRAVE_BROWSER_BRAVE_ACCOUNT_DIALOG_MODE_HOLDER_H_
