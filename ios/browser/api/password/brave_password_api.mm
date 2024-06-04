/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/password/brave_password_api.h"

#include "base/functional/bind.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/ios/browser/api/password/brave_password_observer.h"
#include "brave/ios/browser/api/password/password_store_listener_ios.h"
#include "components/password_manager/core/browser/password_form.h"
#include "components/password_manager/core/browser/password_form_digest.h"
#include "components/password_manager/core/browser/password_store/password_store_consumer.h"
#include "components/password_manager/core/browser/password_store/password_store_interface.h"
#include "ios/web/public/thread/web_thread.h"
#include "net/base/apple/url_conversions.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace brave {
namespace ios {
password_manager::PasswordForm::Scheme
PasswordManagerSchemeFromPasswordFormScheme(PasswordFormScheme scheme) {
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
    self.url = url;
    self.signOnRealm = signOnRealm;
    self.dateCreated = dateCreated;
    self.dateLastUsed = dateLastUsed;
    self.datePasswordChanged = datePasswordChanged;
    self.usernameElement = usernameElement;
    self.usernameValue = usernameValue;
    self.passwordElement = passwordElement;
    self.passwordValue = passwordValue;
    self.isBlockedByUser = isBlockedByUser;
    self.scheme = scheme;
    self.isBlockedByUser = isBlockedByUser;
    self.scheme = scheme;
  }

  return self;
}

- (id)copyWithZone:(NSZone*)zone {
  IOSPasswordForm* passwordFormCopy = [[[self class] allocWithZone:zone] init];

  if (passwordFormCopy) {
    passwordFormCopy.url = self.url;
    passwordFormCopy.signOnRealm = self.signOnRealm;
    passwordFormCopy.dateCreated = self.dateCreated;
    passwordFormCopy.dateLastUsed = self.dateLastUsed;
    passwordFormCopy.datePasswordChanged = self.datePasswordChanged;
    passwordFormCopy.usernameElement = self.usernameElement;
    passwordFormCopy.usernameValue = self.usernameValue;
    passwordFormCopy.passwordElement = self.passwordElement;
    passwordFormCopy.passwordValue = self.passwordValue;
    passwordFormCopy.isBlockedByUser = self.isBlockedByUser;
    passwordFormCopy.scheme = self.scheme;
  }

  return passwordFormCopy;
}

- (void)updatePasswordForm:(NSString*)usernameValue
             passwordValue:(NSString*)passwordValue {
  if ([usernameValue length] != 0) {
    [self setUsernameValue:usernameValue];
  }

  if ([passwordValue length] != 0) {
    [self setPasswordValue:passwordValue];
  }
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

- (id<PasswordStoreListener>)addObserver:(id<PasswordStoreObserver>)observer {
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

  passwordCredentialForm.url = net::GURLWithNSURL(passwordForm.url);

  if ([passwordForm.signOnRealm length] != 0) {
    passwordCredentialForm.signon_realm =
        base::SysNSStringToUTF8(passwordForm.signOnRealm);
  } else {
    passwordCredentialForm.signon_realm = passwordCredentialForm.url.spec();
  }

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
  password_store_->RemoveLogin(FROM_HERE,
                               [self createCredentialForm:passwordForm]);
}

- (void)updateLogin:(IOSPasswordForm*)newPasswordForm
    oldPasswordForm:(IOSPasswordForm*)oldPasswordForm {
  password_store_->UpdateLoginWithPrimaryKey(
      [self createCredentialForm:newPasswordForm],
      [self createCredentialForm:oldPasswordForm]);
}

- (void)getSavedLogins:
    (void (^)(NSArray<IOSPasswordForm*>* results))completion {
  __weak BravePasswordAPI* weakSelf = self;

  auto fetchCredentialsCallback =
      ^(std::vector<std::unique_ptr<password_manager::PasswordForm>> logins) {
        const auto strongSelf = weakSelf;

        NSArray<IOSPasswordForm*>* credentials =
            [strongSelf onLoginsResult:std::move(logins)];
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
  __weak BravePasswordAPI* weakSelf = self;

  auto fetchCredentialsCallback =
      ^(std::vector<std::unique_ptr<password_manager::PasswordForm>> logins) {
        const auto strongSelf = weakSelf;

        NSArray<IOSPasswordForm*>* credentials =
            [strongSelf onLoginsResult:std::move(logins)];
        completion(credentials);
      };

  auto* password_consumer =
      new PasswordStoreConsumerIOS(base::BindOnce(fetchCredentialsCallback));

  password_manager::PasswordFormDigest form_digest_args =
      password_manager::PasswordFormDigest(
          /*scheme*/ brave::ios::PasswordManagerSchemeFromPasswordFormScheme(
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
                     scheme:brave::ios::
                                PasswordFormSchemeFromPasswordManagerScheme(
                                    result->scheme)];

    [loginForms addObject:passwordForm];
  }

  return loginForms;
}

@end
