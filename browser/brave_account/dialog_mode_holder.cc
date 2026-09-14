/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_account/dialog_mode_holder.h"

#include "base/memory/ptr_util.h"
#include "content/public/browser/web_contents.h"

namespace brave_account {

DialogModeHolder::DialogModeHolder(content::WebContents* web_contents,
                                   mojom::DialogMode dialog_mode)
    : content::WebContentsUserData<DialogModeHolder>(*web_contents),
      dialog_mode_(dialog_mode) {}

DialogModeHolder::~DialogModeHolder() = default;

// static
void DialogModeHolder::SetDialogMode(content::WebContents& web_contents,
                                     mojom::DialogMode dialog_mode) {
  web_contents.SetUserData(UserDataKey(), base::WrapUnique(new DialogModeHolder(
                                              &web_contents, dialog_mode)));
}

// static
mojom::DialogMode DialogModeHolder::GetDialogMode(
    content::WebContents& web_contents) {
  auto* holder = FromWebContents(&web_contents);
  return holder ? holder->dialog_mode_ : mojom::DialogMode::kDefault;
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(DialogModeHolder);

}  // namespace brave_account
