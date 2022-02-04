/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/password/password_store_listener_ios.h"

#include "base/check.h"
#include "base/memory/ref_counted.h"
#include "base/strings/sys_string_conversions.h"

#include "components/password_manager/core/browser/password_form.h"
#include "components/password_manager/core/browser/password_store.h"
#include "components/password_manager/core/browser/password_store_interface.h"
#include "net/base/mac/url_conversions.h"
#include "url/gurl.h"

#include "brave/ios/browser/api/password/brave_password_api.h"
#include "brave/ios/browser/api/password/brave_password_observer.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface IOSPasswordForm (Private)

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
                     scheme:(PasswordFormScheme)scheme; 
@end

namespace brave {
namespace ios {

PasswordStoreListenerIOS::PasswordStoreListenerIOS(
    id<PasswordStoreObserver> observer,
    scoped_refptr<password_manager::PasswordStoreInterface> store)
    : observer_(observer), store_(store) {
  DCHECK(observer_);
  DCHECK(store_);
  if (store_)
    store_->AddObserver(this);
}

PasswordStoreListenerIOS::~PasswordStoreListenerIOS() {
DCHECK(store_);
  if (store_)
    store_->RemoveObserver(this);
}

void PasswordStoreListenerIOS::OnLoginsChanged(
    password_manager::PasswordStoreInterface* /*store*/,
    const password_manager::PasswordStoreChangeList& /*changes*/) {

}

void PasswordStoreListenerIOS::OnLoginsRetained(
    password_manager::PasswordStoreInterface* /*store*/,
    const std::vector<password_manager::PasswordForm>& /*retained_passwords*/) {

}

}  // namespace ios
}  // namespace brave

@interface PasswordStoreListenerImpl () {
  std::unique_ptr<brave::ios::PasswordStoreListenerIOS> observer_;
  scoped_refptr<password_manager::PasswordStoreInterface> store_;
}
@end

@implementation PasswordStoreListenerImpl
- (instancetype)init:(id<PasswordStoreObserver>)observer
      passwordStore:(void*)store {
  if ((self = [super init])) {
    observer_ = std::make_unique<brave::ios::PasswordStoreListenerIOS>(
        observer, static_cast<scoped_refptr<password_manager::PasswordStoreInterface>>(store));

    store_ = static_cast<scoped_refptr<password_manager::PasswordStoreInterface>>(store);
  }
  return self;
}

- (void)dealloc {
  [self destroy];
}

- (void)destroy {
  observer_.reset();
}
@end