/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

 #ifndef BRAVE_BROWSER_UI_WEBUI_PSST_PSST_DIALOG_TAB_HELPER_DELEGATE_IMPL_H_
 #define BRAVE_BROWSER_UI_WEBUI_PSST_PSST_DIALOG_TAB_HELPER_DELEGATE_IMPL_H_
 
 #include <string>
 #include <vector>
 
 #include "base/values.h"
 #include "brave/components/psst/browser/content/psst_tab_helper.h"
 #include "brave/components/psst/browser/core/psst_consent_dialog.mojom.h"
 #include "mojo/public/cpp/bindings/receiver_set.h"
 
 class PsstDialogTabHelperDelegateImpl : public psst::PsstTabHelper::Delegate,
                                         public psst_consent_dialog::mojom::PsstConsentHelper {
  public:
  PsstDialogTabHelperDelegateImpl();
   ~PsstDialogTabHelperDelegateImpl() override;
 
   // psst::PsstTabHelper::Delegate
   void ShowPsstConsentDialog(content::WebContents* contents,
                              bool prompt_for_new_version,
                              base::Value::List requests,
                              ConsentCallback yes_cb,
                              ConsentCallback no_cb,
                              base::OnceClosure never_ask_me_callback) override;
   void SetProgressValue(content::WebContents* contents,
                         const double value) override;
   void SetRequestDone(content::WebContents* contents,
                       const std::string& url,
                       const bool is_error) override;
   void SetCompletedView(content::WebContents* contents,
                         const std::vector<std::string>& applied_checks,
                         const std::vector<std::string>& errors,
                         ShareCallback share_cb) override;
   void Close(content::WebContents* contents) override;

   private:

   void SetClientPage(::mojo::PendingRemote<psst_consent_dialog::mojom::PsstConsentDialog> dialog) override;


   mojo::Remote<psst_consent_dialog::mojom::PsstConsentDialog> client_page_;
 };
 
 #endif  // BRAVE_BROWSER_UI_WEBUI_PSST_PSST_DIALOG_TAB_HELPER_DELEGATE_IMPL_H_
 