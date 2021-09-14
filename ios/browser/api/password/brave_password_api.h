/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_HISTORY_BRAVE_PASSWORD_API_H_
#define BRAVE_IOS_BROWSER_API_HISTORY_BRAVE_PASSWORD_API_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

// @protocol PasswordStoreObserver;
// @protocol PasswordStoreListener;

@class IOSPasswordForm;

NS_SWIFT_NAME(PasswordForm)
OBJC_EXPORT
@interface IOSPasswordForm : NSObject

@property(nonatomic, strong, copy) NSURL* url;
@property(nonatomic, strong, copy) NSString* signOnRealm;
@property(nonatomic, nullable, copy) NSDate* dateCreated;
@property(nonatomic, nullable, copy) NSString* usernameValue;
@property(nonatomic, nullable, copy) NSString* passwordValue;
@property(nonatomic, assign, readonly) bool isBlockedByUser;

/// Password Form Constructor used with BravePasswordAPI
/// @param url - Primary data used by the PasswordManager to decide (in longest
///        matching prefix fashion) whether or not a given PasswordForm result from
///        the database is a good fit for a particular form on a page
/// @param signOnRealm - The "Realm" for the sign-on. This is scheme, host, port for SCHEME_HTML
///        The signon_realm is effectively the primary key used for retrieving
///        data from the database, so it must not be empty                         
/// @param usernameValue - The string represantation of the username
/// @param passwordValue - The string represantation of the password
- (instancetype)initWithURL:(NSURL*)url
                signOnRealm:(NSString*)signOnRealm
                dateCreated:(nullable NSDate*)dateCreated
              usernameValue:(nullable NSString*)usernameValue
              passwordValue:(nullable NSString*)passwordValue
            isBlockedByUser:(bool)isBlockedByUser;
@end

NS_SWIFT_NAME(BravePasswordAPI)
OBJC_EXPORT
@interface BravePasswordAPI : NSObject

// - (id<PasswordStoreListener>)addObserver:(id<PasswordStoreObserver>)observer;
// - (void)removeObserver:(id<PasswordStoreListener>)observer;

- (instancetype)init NS_UNAVAILABLE;

/// Add Login Method that add form using password store
/// @param passwordForm - List of Password Forms to be added
- (void)addLogins:(NSArray<IOSPasswordForm*>*)passwordFormList;

/// Remove Selected Password Forms
/// @param passwordForm - Password Form to be removed from the store
- (void)removeLogins:(NSArray<IOSPasswordForm*>*)passwordFormList;

/// Update List of Password Form
/// @param passwordForm - List of Password Form to be updated inside the store
- (void)updateLogins:(NSArray<IOSPasswordForm*>*)passwordFormList;

/// Fetch Function that will return list of Password Forms from saved password presenter
- (NSArray<IOSPasswordForm*>*)getSavedLogins;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_HISTORY_BRAVE_HISTORY_API_H_
