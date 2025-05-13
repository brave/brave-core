/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/psst/psst_dialog_tab_helper_delegate_impl.h"

namespace psst {

PsstDialogTabHelperDelegateImpl::PsstDialogTabHelperDelegateImpl(
    content::WebContents* contents)
    : web_contents_(contents) {}

PsstDialogTabHelperDelegateImpl::~PsstDialogTabHelperDelegateImpl() = default;

void PsstDialogTabHelperDelegateImpl::Show() {
  // Here must be implemented opening of the dialog
}

}  // namespace psst
