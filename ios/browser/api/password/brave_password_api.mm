/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/password/brave_password_api.h"

#include "base/bind.h"
#include "base/strings/sys_string_conversions.h"
#include "components/password_manager/core/browser/password_form.h"

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
                dateCreated:(nullable NSDate*)dateCreated
              usernameValue:(nullable NSString*)usernameValue
              passwordValue:(nullable NSString*)passwordValue
            isBlockedByUser:(bool)isBlockedByUser {
  if ((self = [super init])) {
    [self setUrl:url];

    [self setSignOnRealm:signOnRealm];

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

#pragma mark - BravePasswordAPI

@interface BravePasswordAPI () {

}
@end

@implementation BravePasswordAPI

- (instancetype)init {
  if ((self = [super init])) {
    DCHECK_CURRENTLY_ON(web::WebThread::UI);

  }
  return self;
}

- (void)dealloc {

}

// - (id<PasswordStoreListener>)addObserver:(id<PasswordStoreObserver>)observer {
//   return [[PasswordStoreListenerImpl alloc] init:observer];
// }

// - (void)removeObserver:(id<PasswordStoreListener>)observer {
//   [observer destroy];
// }

- (void)addLogins:(NSArray<IOSPasswordForm*>*)passwordFormList {}

- (void)removeLogins:(NSArray<IOSPasswordForm*>*)passwordFormList {}

- (void)updateLogins:(NSArray<IOSPasswordForm*>*)passwordFormList {}

- (NSArray<IOSPasswordForm*>*)getSavedLogins {
  return [NSArray array]
}
@end
