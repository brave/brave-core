/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/password/brave_password_api.h"

#include "base/bind.h"
#include "base/memory/ref_counted.h"
#include "base/notreached.h"
#include "base/run_loop.h"
#include "base/strings/sys_string_conversions.h"

#include "components/keyed_service/core/service_access_type.h"
#include "components/password_manager/core/browser/password_form.h"
#include "components/password_manager/core/browser/password_form_digest.h"
#include "components/password_manager/core/browser/password_store.h"
#include "components/password_manager/core/browser/password_store_consumer.h"

#include "ios/web/public/thread/web_thread.h"
#include "net/base/mac/url_conversions.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

// #include "brave/ios/browser/api/history/brave_password_observer.h"
// #include "brave/ios/browser/api/history/password_store_listener_ios.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace brave {
namespace ios {
password_manager::PasswordForm::Scheme PasswordFormSchemeForPasswordFormDigest(
    PasswordFormScheme scheme) {
  switch (scheme) {
    case PasswordFormSchemeTypeHtml:
      return password_manager::PasswordForm::Scheme::kHtml;
    case PasswordFormSchemeTypeBasic:
      return password_manager::PasswordForm::Scheme::kBasic;
    case PasswordFormSchemeTypeDigest:
      return password_manager::PasswordForm::Scheme::kDigest;
    case PasswordFormSchemeTypeOther:
      return password_manager::PasswordForm::Scheme::kOther;
    case PasswordFormSchemeUsernameOnly:
      return password_manager::PasswordForm::Scheme::kUsernameOnly;
    default:
      return password_manager::PasswordForm::Scheme::kHtml;
  }
}

PasswordFormScheme PasswordFormSchemeFromPasswordManagerScheme(
    password_manager::PasswordForm::Scheme scheme) {
  switch (scheme) {
    case password_manager::PasswordForm::Scheme::kHtml:
      return PasswordFormSchemeTypeHtml;
    case password_manager::PasswordForm::Scheme::kBasic:
      return PasswordFormSchemeTypeBasic;
    case password_manager::PasswordForm::Scheme::kDigest:
      return PasswordFormSchemeTypeDigest;
    case password_manager::PasswordForm::Scheme::kOther:
      return PasswordFormSchemeTypeOther;
    case password_manager::PasswordForm::Scheme::kUsernameOnly:
      return PasswordFormSchemeUsernameOnly;
    default:
      return PasswordFormSchemeTypeHtml;
  }
}
}  // namespace ios
}  // namespace brave

#pragma mark - IOSPasswordForm

@interface IOSPasswordForm () {
  GURL gurl_;
  std::string signon_realm_;
  base::Time date_created_;
  std::u16string username_value_;
  std::u16string password_value_;
  password_manager::PasswordForm::Scheme password_form_scheme_;
}
@end

@implementation IOSPasswordForm

- (instancetype)initWithURL:(NSURL*)url
                signOnRealm:(NSString*)signOnRealm
                dateCreated:(NSDate*)dateCreated
              usernameValue:(NSString*)usernameValue
              passwordValue:(NSString*)passwordValue
            isBlockedByUser:(bool)isBlockedByUser
                     scheme:(PasswordFormScheme)scheme {
  if ((self = [super init])) {
    [self setUrl:url];

    if (signOnRealm) {
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

    password_form_scheme_ =
        brave::ios::PasswordFormSchemeForPasswordFormDigest(scheme);
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
  return base::SysUTF8ToNSString(signon_realm_);
}

- (void)setDateCreated:(NSDate*)dateCreated {
  date_created_ = base::Time::FromNSDate(dateCreated);
}

- (NSDate*)dateCreated {
  return date_created_.ToNSDate();
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
  return base::SysUTF16ToNSString(password_value_);
}
- (void)setPasswordFormScheme:(PasswordFormScheme)passwordFormScheme {
  password_form_scheme_ =
      brave::ios::PasswordFormSchemeForPasswordFormDigest(passwordFormScheme);
}

- (PasswordFormScheme)passwordFormScheme {
  return brave::ios::PasswordFormSchemeFromPasswordManagerScheme(
      password_form_scheme_);
}
@end

#pragma mark - PasswordStoreConsumerHelper

class PasswordStoreConsumerHelper
    : public password_manager::PasswordStoreConsumer {
 public:
  PasswordStoreConsumerHelper() {}

  void OnGetPasswordStoreResults(
      std::vector<std::unique_ptr<password_manager::PasswordForm>> results)
      override {
    result_.swap(results);
    run_loop_.Quit();
  }

  std::vector<std::unique_ptr<password_manager::PasswordForm>> WaitForResult() {
    DCHECK(!run_loop_.running());
    run_loop_.Run();
    return std::move(result_);
  }

 private:
  base::RunLoop run_loop_;
  std::vector<std::unique_ptr<password_manager::PasswordForm>> result_;

  DISALLOW_COPY_AND_ASSIGN(PasswordStoreConsumerHelper);
};

#pragma mark - BravePasswordAPI

@interface BravePasswordAPI () {
  scoped_refptr<password_manager::PasswordStore> password_store_;
}
@end

@implementation BravePasswordAPI

- (instancetype)initWithPasswordStore:
    (scoped_refptr<password_manager::PasswordStore>)passwordStore {
  if ((self = [super init])) {
    DCHECK_CURRENTLY_ON(web::WebThread::UI);

    password_store_ = passwordStore;
  }
  return self;
}

- (void)dealloc {
  password_store_ = nil;
}

// - (id<PasswordStoreListener>)addObserver:(id<PasswordStoreObserver>)observer
// {
//   return [[PasswordStoreListenerImpl alloc] init:observer];
// }

// - (void)removeObserver:(id<PasswordStoreListener>)observer {
//   [observer destroy];
// }

- (void)addLogin:(IOSPasswordForm*)passwordForm {
  password_store_->AddLogin([self createCredentialForm:passwordForm]);
}

- (password_manager::PasswordForm)createCredentialForm:
    (IOSPasswordForm*)passwordForm {
  // Store a PasswordForm representing a PasswordCredential.
  password_manager::PasswordForm passwordCredentialForm;

  if (passwordForm.usernameValue) {
    passwordCredentialForm.username_value =
        base::SysNSStringToUTF16(passwordForm.usernameValue);
  }

  if (passwordForm.passwordValue) {
    passwordCredentialForm.password_value =
        base::SysNSStringToUTF16(passwordForm.passwordValue);
  }

  passwordCredentialForm.url = net::GURLWithNSURL(passwordForm.url).GetOrigin();

  if (passwordForm.signOnRealm) {
    passwordCredentialForm.signon_realm =
        base::SysNSStringToUTF8(passwordForm.signOnRealm);
  } else {
    passwordCredentialForm.signon_realm = passwordCredentialForm.url.spec();
  }

  if (passwordForm.usernameValue && !passwordForm.passwordValue) {
    passwordCredentialForm.scheme =
        password_manager::PasswordForm::Scheme::kUsernameOnly;
  } else {
    passwordCredentialForm.scheme =
        password_manager::PasswordForm::Scheme::kHtml;
  }

  return passwordCredentialForm;
}

- (void)removeLogin:(IOSPasswordForm*)passwordForm {
  password_store_->RemoveLogin([self createCredentialForm:passwordForm]);
}

- (void)updateLogin:(IOSPasswordForm*)newPasswordForm
    oldPasswordForm:(IOSPasswordForm*)oldPasswordForm {
  password_store_->UpdateLoginWithPrimaryKey(
      [self createCredentialForm:newPasswordForm],
      [self createCredentialForm:oldPasswordForm]);
}

- (NSArray<IOSPasswordForm*>*)getSavedLogins {
  PasswordStoreConsumerHelper password_consumer;
  password_store_->GetAllLogins(&password_consumer);

  std::vector<std::unique_ptr<password_manager::PasswordForm>> credentials =
      password_consumer.WaitForResult();

  return [self onLoginsResult:std::move(credentials)];
}

- (NSArray<IOSPasswordForm*>*)getSavedLoginsForURL:(NSURL*)url
                                        formScheme:
                                            (PasswordFormScheme)formScheme {
  PasswordStoreConsumerHelper password_consumer;

  password_manager::PasswordFormDigest form_digest_args =
      password_manager::PasswordFormDigest(
          /*scheme*/ brave::ios::PasswordFormSchemeForPasswordFormDigest(
              formScheme),
          /*signon_realm*/ net::GURLWithNSURL(url).spec(),
          /*url*/ net::GURLWithNSURL(url));

  password_store_->GetLogins(form_digest_args, &password_consumer);

  std::vector<std::unique_ptr<password_manager::PasswordForm>> credentials =
      password_consumer.WaitForResult();

  return [self onLoginsResult:std::move(credentials)];
}

- (NSArray<IOSPasswordForm*>*)onLoginsResult:
    (std::vector<std::unique_ptr<password_manager::PasswordForm>>)results {
  NSMutableArray<IOSPasswordForm*>* loginForms = [[NSMutableArray alloc] init];

  for (const auto& result : results) {
    IOSPasswordForm* passwordForm = [[IOSPasswordForm alloc]
            initWithURL:net::NSURLWithGURL(result->url)
            signOnRealm:base::SysUTF8ToNSString(result->signon_realm)
            dateCreated:result->date_created.ToNSDate()
          usernameValue:base::SysUTF16ToNSString(result->username_value)
          passwordValue:base::SysUTF16ToNSString(result->password_value)
        isBlockedByUser:result->blocked_by_user
                 scheme:brave::ios::PasswordFormSchemeFromPasswordManagerScheme(
                            result->scheme)];

    [loginForms addObject:passwordForm];
  }

  return loginForms;
}

@end
