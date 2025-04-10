/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

 #include "brave/browser/ui/webui/psst/psst_dialog_tab_helper_delegate_impl.h"

 #include "brave/browser/ui/webui/psst/brave_psst_dialog.h"

 PsstDialogTabHelperDelegateImpl::PsstDialogTabHelperDelegateImpl() = default;
 
 PsstDialogTabHelperDelegateImpl::~PsstDialogTabHelperDelegateImpl() = default;

 void PsstDialogTabHelperDelegateImpl::ShowPsstConsentDialog(
     content::WebContents* contents,
     bool prompt_for_new_version,
     base::Value::List requests,
     ConsentCallback yes_cb,
     ConsentCallback no_cb,
     base::OnceClosure never_ask_me_callback) {
    psst::OpenPsstDialog(contents);
}

 void PsstDialogTabHelperDelegateImpl::SetProgressValue(
     content::WebContents* contents,
     const double value) {
   NOTIMPLEMENTED();
 }
 
 void PsstDialogTabHelperDelegateImpl::SetRequestDone(
     content::WebContents* contents,
     const std::string& url,
     const bool is_error) {
    
 //   FireWebUIListener("psst.onSetRequestDone", 0);
 }
 
 void PsstDialogTabHelperDelegateImpl::SetCompletedView(
     content::WebContents* contents,
     const std::vector<std::string>& applied_checks,
     const std::vector<std::string>& errors,
     ShareCallback share_cb) {}
 
 void PsstDialogTabHelperDelegateImpl::Close(content::WebContents* contents) {
   NOTIMPLEMENTED();
 }


 void PsstDialogTabHelperDelegateImpl::SetClientPage(::mojo::PendingRemote<psst_consent_dialog::mojom::PsstConsentDialog> dialog) {
  client_page_.Bind(std::move(dialog));
 }
 