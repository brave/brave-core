/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/password/brave_password_api.h"

#include "base/bind.h"
#include "base/strings/sys_string_conversions.h"

#include "components/keyed_service/core/service_access_type.h"
#include "components/password_manager/core/browser/password_form.h"
#include "components/password_manager/core/browser/password_store.h"
#include "components/password_manager/core/browser/password_store_consumer.h"

#include "ios/chrome/browser/passwords/ios_chrome_password_store_factory.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state_manager.h"

#include "ios/web/public/thread/web_thread.h"
#include "net/base/mac/url_conversions.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

// #include "brave/ios/browser/api/history/brave_password_observer.h"
// #include "brave/ios/browser/api/history/password_store_listener_ios.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#pragma mark - IOSPasswordForm

@interface IOSPasswordForm () {
  GURL gurl_;
  std::string signon_realm_;
  base::Time date_created_;
  std::u16string username_value_;
  std::u16string password_value_;
}
@end

@implementation IOSPasswordForm

- (instancetype)initWithURL:(NSURL*)url
                signOnRealm:(NSString*)signOnRealm
                dateCreated:(NSDate*)dateCreated
              usernameValue:(NSString*)usernameValue
              passwordValue:(NSString*)passwordValue
            isBlockedByUser:(bool)isBlockedByUser {
  if ((self = [super init])) {
    [self setUrl:url];

    if (signONRealm) {}
      [self setSignOnRealm:signOnRealm];
    }

    if (dateCreated) {
      [self setDateCreated:dateCreated];
    }

    if (usernameValue) {
      [self setUsernameValue:usernameValue];
    }
    
    if (passwordValue) {
      [self setPasswordValue:passwordValue];
    }

    self.isBlockedByUser = isBlockedByUser;
  }

  return self;
}

- (void)setUrl:(NSURL*)url {
  gurl_ = net::GURLWithNSURL(url);
}

- (NSURL*)url {
  return net::NSURLWithGURL(gurl_);
}

- (void)setSignOnRealm:(NSString*)signOnRealm {
  signon_realm_ = base::SysNSStringToUTF8(signOnRealm);
}

- (NSString*)signOnRealm {
  return base::SysNSStringToUTF8(signon_realm_);
}

- (void)setDateCreated:(NSDate*)dateCreated {
  date_created_ = base::Time::FromNSDate(dateCreated);
}

- (NSDate*)dateAdded {
  return date_added_.ToNSDate();
}

- (void)setUsernameValue:(NSString*)usernameValue {
  username_value_ = base::SysNSStringToUTF16(usernameValue);
}

- (NSString*)usernameValue {
  return base::SysUTF16ToNSString(username_value_);
}

- (void)setPasswordValue:(NSString*)passwordValue {
  password_value_ = base::SysNSStringToUTF16(passwordValue);
}

- (NSString*)passwordValue {
  return base::SysUTF16ToNSString(passwordValue);
}
@end

#pragma mark - PasswordStoreConsumerHelper

class PasswordStoreConsumerHelper: public password_manager::PasswordStoreConsumer {
 public:
  PasswordStoreConsumerHelper() {}

  void OnGetPasswordStoreResults(
      std::vector<std::unique_ptr<password_manager::PasswordForm>> results) override {
    result_.swap(results);
    run_loop_.Quit();
  }

  std::vector<std::unique_ptr<password_manager::PasswordForm>> WaitForResult() {
    DCHECK(!run_loop_.running());
    content::RunThisRunLoop(&run_loop_);
    return std::move(result_);
  }

 private:
  base::RunLoop run_loop_;
  std::vector<std::unique_ptr<PasswordForm>> result_;

  DISALLOW_COPY_AND_ASSIGN(PasswordStoreConsumerHelper);
};

#pragma mark - BravePasswordAPI

@interface BravePasswordAPI () {
  scoped_refptr<password_manager::PasswordStore> password_store_;  
}
@end

@implementation BravePasswordAPI

- (instancetype)init {
  if ((self = [super init])) {
    DCHECK_CURRENTLY_ON(web::WebThread::UI);

    ios::ChromeBrowserStateManager* browserStateManager =
      GetApplicationContext()->GetChromeBrowserStateManager();
    ChromeBrowserState* chromeBrowserState =
      browserStateManager->GetLastUsedBrowserState();

    password_store_ = IOSChromePasswordStoreFactory::GetForBrowserState(
      chromeBrowserState),
      ServiceAccessType::EXPLICIT_ACCESS)
        .get();

  }
  return self;
}

- (void)dealloc {
  password_store_ = nil
}

// - (id<PasswordStoreListener>)addObserver:(id<PasswordStoreObserver>)observer {
//   return [[PasswordStoreListenerImpl alloc] init:observer];
// }

// - (void)removeObserver:(id<PasswordStoreListener>)observer {
//   [observer destroy];
// }

- (void)addLogin:(IOSPasswordForm*)passwordForm {
  password_store_->AddLogin([self createCredentialForm:passwordForm]);
}

-(password_manager::PasswordForm)createCredentialForm:(IOSPasswordForm*)passwordForm {
  // Store a PasswordForm representing a PasswordCredential.
  password_manager::PasswordForm passwordCredentialForm;

  if (passwordForm.usernameValue) {
    passwordCredentialForm.username_value = base::SysNSStringToUTF16(passwordForm.usernameValue);
  }

  if (passwordForm.passwordValue) {
    passwordCredentialForm.password_value = base::SysNSStringToUTF16(passwordForm.passwordValue);
  }

  passwordCredentialForm.url = net::GURLWithNSURL(passwordForm.url).GetOrigin();

  if (passwordForm.signOnRealm) {
    passwordCredentialForm.signOnRealm = base::SysNSStringToUTF8(passwordForm.signOnRealm);
  } else {
    passwordCredentialForm.signon_realm = passwordCredentialForm.url.spec();
  } 

  passwordCredentialForm.scheme = password_manager::PasswordForm::Scheme::kHtml;

  return passwordCredentialForm;
}

- (void)removeLogin:(IOSPasswordForm*)passwordForm {}

- (void)updateLogin:(IOSPasswordForm*)passwordForm {}

- (NSArray<IOSPasswordForm*>*)getSavedLogins {
  PasswordStoreConsumerHelper password_consumer;
  password_store_->GetAllLogins(&password_consumer);

  std::vector<std::unique_ptr<password_manager::PasswordForm>> credentials =
      password_consumer.WaitForResult();

  return [self onLoginsResult:credentials];
}

- (NSArray<IOSPasswordForm*>*)onLoginsResult:(std::vector<std::unique_ptr<password_manager::PasswordForm>>)results {
  NSMutableArray<IOSPasswordForm*>* loginForms = [[NSMutableArray alloc] init];

  for (const auto& result : results) {
    IOSPasswordForm* passwordForm = [[IOSPasswordForm alloc]
        initWithURL:net::NSURLWithGURL(result.url)
        signOnRealm:base::SysNSStringToUTF8(result.signon_realm)
        dateCreated:result.date_created.ToNSDate()
      usernameValue:base::SysNSStringToUTF16(result.username_value)
      passwordValue:base::SysNSStringToUTF16(result.password_value)
    isBlockedByUser:result.blocked_by_user];

    [loginForms addObject:passwordForm];
  }
}
@end
