// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_ACCOUNT_DIALOG_MODE_HOLDER_H_
#define BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_ACCOUNT_DIALOG_MODE_HOLDER_H_

#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "ios/web/public/web_state_user_data.h"

namespace brave_account {

// WebStateUserData that holds onto the dialog mode the brave://account WebUI
// page was opened in. Set by the host surface that opens the page, before the
// page is loaded; read back when the page asks via
// `mojom::DialogController::GetDialogMode()`.
//
// iOS counterpart of browser/brave_account/dialog_mode_holder.h.
class DialogModeHolder : public web::WebStateUserData<DialogModeHolder> {
 public:
  void SetDialogMode(mojom::DialogMode dialog_mode) {
    dialog_mode_ = dialog_mode;
  }

  mojom::DialogMode dialog_mode() const { return dialog_mode_; }

 private:
  friend class web::WebStateUserData<DialogModeHolder>;

  explicit DialogModeHolder(web::WebState*);

  mojom::DialogMode dialog_mode_ = mojom::DialogMode::kDefault;
};

}  // namespace brave_account

#endif  // BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_ACCOUNT_DIALOG_MODE_HOLDER_H_
