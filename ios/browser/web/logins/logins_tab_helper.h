// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_WEB_LOGINS_LOGINS_TAB_HELPER_H_
#define BRAVE_IOS_BROWSER_WEB_LOGINS_LOGINS_TAB_HELPER_H_

#import <Foundation/Foundation.h>

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "ios/web/public/web_state_user_data.h"

@protocol LoginsTabHelperBridge;
@class BravePasswordAPI;
@class IOSPasswordForm;

// Tab helper for handling password operations triggered by the logins
// JavaScript feature. Implementations bridge to the platform password API.
class LoginsTabHelper : public web::WebStateUserData<LoginsTabHelper> {
 public:
  ~LoginsTabHelper() override;

  // Creates a LoginsTabHelper for |web_state| and wires |bridge| as the
  // delegate, but only when the feature flags that enable the
  // JavaScriptFeature-based logins path are set. No-ops otherwise.
  static void MaybeCreateForWebState(web::WebState* web_state,
                                     id<LoginsTabHelperBridge> bridge);

  void SetBridge(id<LoginsTabHelperBridge> bridge);

  // Called when a page form requests saved logins for |form_origin| /
  // |action_origin|. |is_main_frame| mirrors ScriptMessage::is_main_frame()
  // and is used to strip passwords for cross-origin subframe requests.
  // The implementation invokes |callback| with a JSON array string of login
  // dictionaries keyed by hostname, formSubmitURL, httpRealm, username,
  // password, usernameField, and passwordField.
  void GetSavedLogins(const std::string& form_origin,
                      const std::string& action_origin,
                      bool is_main_frame,
                      base::OnceCallback<void(std::string)> callback);

  // Called when a form is submitted with credentials. See
  // LoginsTabHelperBridge::handleFormSubmit: for the |credentials_json| format.
  void HandleFormSubmit(std::string credentials_json);

 private:
  friend class web::WebStateUserData<LoginsTabHelper>;

  explicit LoginsTabHelper(web::WebState* web_state,
                           BravePasswordAPI* password_api);

  void OnGetSavedLogins(base::OnceCallback<void(std::string)> callback,
                        std::string action_origin,
                        bool is_main_frame,
                        std::string form_origin,
                        NSArray<IOSPasswordForm*>* forms);

  __weak id<LoginsTabHelperBridge> bridge_;
  raw_ptr<web::WebState> web_state_;
  BravePasswordAPI* password_api_;

  base::WeakPtrFactory<LoginsTabHelper> weak_ptr_factory_{this};
};

#endif  // BRAVE_IOS_BROWSER_WEB_LOGINS_LOGINS_TAB_HELPER_H_
