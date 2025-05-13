/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PSST_PSST_DIALOG_TAB_HELPER_DELEGATE_IMPL_H_
#define BRAVE_BROWSER_PSST_PSST_DIALOG_TAB_HELPER_DELEGATE_IMPL_H_

#include "base/memory/raw_ptr.h"
#include "brave/components/psst/browser/core/psst_dialog_delegate.h"
#include "content/public/browser/web_contents.h"

namespace psst {

// Represents the implementation of the dialog delegate
class PsstDialogTabHelperDelegateImpl : public PsstDialogDelegate {
 public:
  explicit PsstDialogTabHelperDelegateImpl(content::WebContents* contents);
  ~PsstDialogTabHelperDelegateImpl() override;

  // PsstDialogDelegate overrides
  void Show() override;

 private:
  raw_ptr<content::WebContents> web_contents_;
};

}  // namespace psst

#endif  // BRAVE_BROWSER_PSST_PSST_DIALOG_TAB_HELPER_DELEGATE_IMPL_H_
