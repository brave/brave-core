/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_player/brave_player_tab_helper_delegate_impl.h"

BravePlayerTabHelperDelegateImpl::BravePlayerTabHelperDelegateImpl() = default;

BravePlayerTabHelperDelegateImpl::~BravePlayerTabHelperDelegateImpl() = default;

#if !defined(TOOLKIT_VIEWS)
void BravePlayerTabHelperDelegateImpl::ShowAdBlockAdjustmentSuggestion(
    content::WebContents* contents) {
  NOTIMPLEMENTED();
}
#endif
