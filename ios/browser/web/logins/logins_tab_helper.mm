// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/web/logins/logins_tab_helper.h"

#include "base/feature_list.h"
#include "base/functional/callback_helpers.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/ios/browser/api/password/brave_password_api+private.h"
#include "brave/ios/browser/api/password/brave_password_api.h"
#include "brave/ios/browser/ui/web_view/features.h"
#include "brave/ios/browser/web/logins/logins_tab_helper_bridge.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/password_manager/core/browser/password_store/password_store_interface.h"
#include "ios/chrome/browser/passwords/model/ios_chrome_profile_password_store_factory.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/web_state.h"
#include "net/base/apple/url_conversions.h"
#include "url/gurl.h"
#include "url/origin.h"

LoginsTabHelper::LoginsTabHelper(web::WebState* web_state,
                                 BravePasswordAPI* password_api)
    : web_state_(web_state), password_api_(password_api) {}

LoginsTabHelper::~LoginsTabHelper() {}

// static
void LoginsTabHelper::MaybeCreateForWebState(web::WebState* web_state,
                                             id<LoginsTabHelperBridge> bridge) {
  if (!base::FeatureList::IsEnabled(
          brave::features::kUseProfileWebViewConfiguration) ||
      base::FeatureList::IsEnabled(
          brave::features::kUseChromiumWebViewsAutofill)) {
    return;
  }
  ProfileIOS* profile =
      ProfileIOS::FromBrowserState(web_state->GetBrowserState());
  auto password_store = IOSChromeProfilePasswordStoreFactory::GetForProfile(
      profile, ServiceAccessType::EXPLICIT_ACCESS);
  CreateForWebState(web_state, [[BravePasswordAPI alloc]
                                   initWithPasswordStore:password_store.get()]);
  if (auto* helper = FromWebState(web_state)) {
    helper->SetBridge(bridge);
  }
}

void LoginsTabHelper::SetBridge(id<LoginsTabHelperBridge> bridge) {
  bridge_ = bridge;
}

void LoginsTabHelper::GetSavedLogins(
    const std::string& form_origin,
    const std::string& action_origin,
    bool is_main_frame,
    base::OnceCallback<void(std::string)> callback) {
  NSURL* url = net::NSURLWithGURL(
      web_state_->GetLastCommittedURLIfTrusted().value_or(GURL()));
  if (!url) {
    std::move(callback).Run("[]");
    return;
  }

  auto completion_handler = base::CallbackToBlock(base::BindOnce(
      &LoginsTabHelper::OnGetSavedLogins, weak_ptr_factory_.GetWeakPtr(),
      std::move(callback), action_origin, is_main_frame, form_origin));

  [password_api_ getSavedLoginsForURL:url
                           formScheme:PasswordFormSchemeTypeHtml
                           completion:completion_handler];
}

void LoginsTabHelper::OnGetSavedLogins(
    base::OnceCallback<void(std::string)> callback,
    std::string action_origin,
    bool is_main_frame,
    std::string form_origin,
    NSArray<IOSPasswordForm*>* forms) {
  NSMutableArray<NSDictionary<NSString*, NSString*>*>* login_dicts =
      [[NSMutableArray alloc] init];

  // For non-main frames, only return credentials when the frame's origin
  // matches the tab's committed URL (mirrors checkIsSameFrame in
  // LoginsScriptHandler.swift). Password is always stripped for subframes.
  url::Origin tab_origin =
      url::Origin::Create(web_state_->GetLastCommittedURL());
  bool same_origin = tab_origin.IsSameOriginWith(GURL(form_origin));

  if (!is_main_frame && !same_origin) {
    std::move(callback).Run("[]");
    return;
  }

  for (IOSPasswordForm* form in forms) {
    // For non-main frames with a matching origin, return the username but
    // omit the password. Chromium does the same on iOS.
    NSString* password = is_main_frame ? (form.passwordValue ?: @"") : @"";

    [login_dicts addObject:@{
      @"hostname" : form.signOnRealm,
      @"formSubmitURL" : base::SysUTF8ToNSString(action_origin),
      @"httpRealm" : @"",
      @"username" : form.usernameValue ?: @"",
      @"password" : password,
      @"usernameField" : form.usernameElement ?: @"",
      @"passwordField" : form.passwordElement ?: @"",
    }];
  }

  NSError* error = nil;
  NSData* json_data = [NSJSONSerialization dataWithJSONObject:login_dicts
                                                      options:0
                                                        error:&error];
  if (!json_data || error) {
    std::move(callback).Run("[]");
    return;
  }

  NSString* json_string = [[NSString alloc] initWithData:json_data
                                                encoding:NSUTF8StringEncoding];
  std::move(callback).Run(base::SysNSStringToUTF8(json_string ?: @"[]"));
}

void LoginsTabHelper::HandleFormSubmit(std::string credentials_json) {
  [bridge_ handleFormSubmit:base::SysUTF8ToNSString(credentials_json)];
}
