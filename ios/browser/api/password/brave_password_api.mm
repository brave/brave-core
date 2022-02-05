/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/password/brave_password_api.h"

#include "base/bind.h"
#include "base/strings/sys_string_conversions.h"

#include "components/password_manager/core/browser/password_form.h"
#include "components/password_manager/core/browser/password_form_digest.h"
#include "components/password_manager/core/browser/password_store_consumer.h"
#include "components/password_manager/core/browser/password_store_interface.h"

#include "ios/web/public/thread/web_thread.h"
#include "net/base/mac/url_conversions.h"
#include "url/gurl.h"

#include "brave/ios/browser/api/password/brave_password_observer.h"
#include "brave/ios/browser/api/password/password_store_listener_ios.h"

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
  base::Time date_last_used_;
  base::Time date_password_changed_;
  std::u16string username_element_;
  std::u16string username_value_;
  std::u16string password_element_;
  std::u16string password_value_;
  password_manager::PasswordForm::Scheme password_form_scheme_;
}
@end

@implementation IOSPasswordForm

- (instancetype)initWithURL:(NSURL*)url
                signOnRealm:(NSString*)signOnRealm
                dateCreated:(NSDate*)dateCreated
               dateLastUsed:(NSDate*)dateLastUsed
        datePasswordChanged:(NSDate*)datePasswordChanged
            usernameElement:(NSString*)usernameElement
              usernameValue:(NSString*)usernameValue
            passwordElement:(NSString*)passwordElement
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

    if (dateLastUsed) {
      [self setDateLastUsed:dateLastUsed];
    }

    if (datePasswordChanged) {
      [self setDatePasswordChanged:datePasswordChanged];
    }
    if (usernameElement) {
      [self setUsernameElement:usernameElement];
    }

    if (usernameValue) {
      [self setUsernameValue:usernameValue];
    }

    if (passwordElement) {
      [self setPasswordElement:passwordElement];
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

- (void)updatePasswordForm:(NSString*)usernameValue 
              passwordValue:(NSString*)passwordValue {

  if ([usernameValue length] != 0) {
    [self setUsernameValue:usernameValue];
  }

  if([passwordValue length] != 0) {
    [self setPasswordValue:passwordValue];
  }
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

- (void)setDateLastUsed:(NSDate*)dateLastUsed {
  date_last_used_ = base::Time::FromNSDate(dateLastUsed);
}

- (void)setDatePasswordChanged:(NSDate*)datePasswordChanged {
  date_created_ = base::Time::FromNSDate(datePasswordChanged);
}

- (NSDate*)dateCreated {
  return date_created_.ToNSDate();
}

- (NSDate*)dateLastUsed {
  return date_last_used_.ToNSDate();
}

- (NSDate*)datePasswordChanged {
  return date_password_changed_.ToNSDate();
}

- (void)setUsernameElement:(NSString*)usernameElement {
  username_element_ = base::SysNSStringToUTF16(usernameElement);
}

- (void)setUsernameValue:(NSString*)usernameValue {
  username_value_ = base::SysNSStringToUTF16(usernameValue);
}

- (NSString*)usernameElement {
  return base::SysUTF16ToNSString(username_element_);
}

- (NSString*)usernameValue {
  return base::SysUTF16ToNSString(username_value_);
}

- (void)setPasswordElement:(NSString*)passwordElement {
  password_element_ = base::SysNSStringToUTF16(passwordElement);
}

- (void)setPasswordValue:(NSString*)passwordValue {
  password_value_ = base::SysNSStringToUTF16(passwordValue);
}

- (NSString*)passwordElement {
  return base::SysUTF16ToNSString(password_element_);
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

#pragma mark - PasswordStoreConsumerIOS

class PasswordStoreConsumerIOS
    : public password_manager::PasswordStoreConsumer {
 public:
  PasswordStoreConsumerIOS(
      base::OnceCallback<
          void(std::vector<std::unique_ptr<password_manager::PasswordForm>>)>
          callback)
      : consumer_callback(std::move(callback)) {}

  base::WeakPtr<PasswordStoreConsumerIOS> GetWeakPtr();

 private:
  base::OnceCallback<void(
      std::vector<std::unique_ptr<password_manager::PasswordForm>>)>
      consumer_callback;

  void OnGetPasswordStoreResults(
      std::vector<std::unique_ptr<password_manager::PasswordForm>> results)
      override;

  base::WeakPtrFactory<PasswordStoreConsumerIOS> weak_ptr_factory_{this};
};

base::WeakPtr<PasswordStoreConsumerIOS> PasswordStoreConsumerIOS::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void PasswordStoreConsumerIOS::OnGetPasswordStoreResults(
    std::vector<std::unique_ptr<password_manager::PasswordForm>> results) {
  std::move(consumer_callback).Run(std::move(results));
  delete this;
}

#pragma mark - BravePasswordAPI

@interface BravePasswordAPI () {
  scoped_refptr<password_manager::PasswordStoreInterface> password_store_;
}
@end

@implementation BravePasswordAPI

- (instancetype)initWithPasswordStore:
    (scoped_refptr<password_manager::PasswordStoreInterface>)passwordStore {
  if ((self = [super init])) {
    DCHECK_CURRENTLY_ON(web::WebThread::UI);

    password_store_ = passwordStore;
  }
  return self;
}

- (void)dealloc {
  password_store_ = nil;
}

- (bool)isAbleToSavePasswords {
  // Returns whether the initialization was successful.
  return password_store_->IsAbleToSavePasswords();
}

- (id<PasswordStoreListener>)addObserver:(id<PasswordStoreObserver>)observer
{
  return [[PasswordStoreListenerImpl alloc] init:observer
                                   passwordStore:password_store_];
}

- (void)removeObserver:(id<PasswordStoreListener>)observer {
  [observer destroy];
}

- (void)addLogin:(IOSPasswordForm*)passwordForm {
  password_store_->AddLogin([self createCredentialForm:passwordForm]);
}

- (password_manager::PasswordForm)createCredentialForm:
    (IOSPasswordForm*)passwordForm {
  // Store a PasswordForm representing a PasswordCredential.
  password_manager::PasswordForm passwordCredentialForm;

  if (passwordForm.usernameElement) {
    passwordCredentialForm.username_element =
        base::SysNSStringToUTF16(passwordForm.usernameElement);
  }

  if (passwordForm.usernameValue) {
    passwordCredentialForm.username_value =
        base::SysNSStringToUTF16(passwordForm.usernameValue);
  }

  if (passwordForm.passwordElement) {
    passwordCredentialForm.password_element =
        base::SysNSStringToUTF16(passwordForm.passwordElement);
  }

  if (passwordForm.passwordValue) {
    passwordCredentialForm.password_value =
        base::SysNSStringToUTF16(passwordForm.passwordValue);
  }

  passwordCredentialForm.url =
      net::GURLWithNSURL(passwordForm.url).DeprecatedGetOriginAsURL();

  if ([passwordForm.signOnRealm length] != 0) {
    passwordCredentialForm.signon_realm =
        base::SysNSStringToUTF8(passwordForm.signOnRealm);
  } else {
    passwordCredentialForm.signon_realm = passwordCredentialForm.url.spec();
  }

  if (passwordForm.dateCreated) {
    passwordCredentialForm.date_created =
        base::Time::FromNSDate(passwordForm.dateCreated);
  } else {
    passwordCredentialForm.date_created = base::Time::Now();
  }

  if (passwordForm.dateLastUsed) {
    passwordCredentialForm.date_last_used =
        base::Time::FromNSDate(passwordForm.dateLastUsed);
  } else {
    passwordCredentialForm.date_last_used = base::Time::Now();
  }

  if (passwordForm.datePasswordChanged) {
    passwordCredentialForm.date_password_modified =
        base::Time::FromNSDate(passwordForm.datePasswordChanged);
  } else {
    passwordCredentialForm.date_password_modified = base::Time::Now();
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

- (void)getSavedLogins:(void (^)(NSArray<IOSPasswordForm*>* results))completion {
  auto fetchCredentialsCallback = ^(std::vector<std::unique_ptr<password_manager::PasswordForm>> logins) {
    NSArray<IOSPasswordForm*>* credentials = [self onLoginsResult:std::move(logins)];
    completion(credentials);
  };

  auto* password_consumer =
      new PasswordStoreConsumerIOS(base::BindOnce(fetchCredentialsCallback));
  password_store_->GetAllLogins(password_consumer->GetWeakPtr());
}

- (void)getSavedLoginsForURL:(NSURL*)url
                  formScheme:(PasswordFormScheme)formScheme
                  completion:
                      (void (^)(NSArray<IOSPasswordForm*>* results))completion {               
  auto fetchCredentialsCallback = ^(std::vector<std::unique_ptr<password_manager::PasswordForm>> logins) {
    NSArray<IOSPasswordForm*>* credentials = [self onLoginsResult:std::move(logins)];
    completion(credentials);
  };

  auto* password_consumer =
      new PasswordStoreConsumerIOS(base::BindOnce(fetchCredentialsCallback));

  password_manager::PasswordFormDigest form_digest_args =
      password_manager::PasswordFormDigest(
          /*scheme*/ brave::ios::PasswordFormSchemeForPasswordFormDigest(
              formScheme),
          /*signon_realm*/ net::GURLWithNSURL(url).spec(),
          /*url*/ net::GURLWithNSURL(url));

  password_store_->GetLogins(form_digest_args, password_consumer->GetWeakPtr());
}

- (NSArray<IOSPasswordForm*>*)onLoginsResult:
    (std::vector<std::unique_ptr<password_manager::PasswordForm>>)results {
  NSMutableArray<IOSPasswordForm*>* loginForms = [[NSMutableArray alloc] init];

  for (const auto& result : results) {
    IOSPasswordForm* passwordForm = [[IOSPasswordForm alloc]
            initWithURL:net::NSURLWithGURL(result->url)
            signOnRealm:base::SysUTF8ToNSString(result->signon_realm)
            dateCreated:result->date_created.ToNSDate()
            dateLastUsed:result->date_last_used.ToNSDate()
            datePasswordChanged:result->date_password_modified.ToNSDate()
        usernameElement:base::SysUTF16ToNSString(result->username_element)
          usernameValue:base::SysUTF16ToNSString(result->username_value)
        passwordElement:base::SysUTF16ToNSString(result->password_element)
          passwordValue:base::SysUTF16ToNSString(result->password_value)
        isBlockedByUser:result->blocked_by_user
                 scheme:brave::ios::PasswordFormSchemeFromPasswordManagerScheme(
                            result->scheme)];

    [loginForms addObject:passwordForm];
  }

  return loginForms;
}

@end
