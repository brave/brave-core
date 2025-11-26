/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ephemeral_storage/ignore_onbeforeunload_web_contents_delegate.h"

#include "chrome/browser/ui/tabs/tab_enums.h"

IgnoreOnBeforeUnloadWebContentsDelegate::
    IgnoreOnBeforeUnloadWebContentsDelegate(TabStripModel* tab_strip_model)
    : tab_strip_model_(tab_strip_model) {}

IgnoreOnBeforeUnloadWebContentsDelegate::
    ~IgnoreOnBeforeUnloadWebContentsDelegate() = default;

void IgnoreOnBeforeUnloadWebContentsDelegate::CloseContents(
    content::WebContents* source) {
  if (!tab_strip_model_) {
    return;
  }
  int index = tab_strip_model_->GetIndexOfWebContents(source);

  if (index != TabStripModel::kNoTab) {
    tab_strip_model_->CloseWebContentsAt(index, TabCloseTypes::CLOSE_NONE);
  }
}

bool IgnoreOnBeforeUnloadWebContentsDelegate::ShouldSuppressDialogs(
    content::WebContents* source) {
  return true;
}
