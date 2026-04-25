/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_PASSWORD_BRAVE_PASSWORD_API_H_
#define BRAVE_IOS_BROWSER_API_PASSWORD_BRAVE_PASSWORD_API_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSUInteger, PasswordFormScheme) {
  PasswordFormSchemeTypeHtml = 0,
  PasswordFormSchemeTypeBasic,
  PasswordFormSchemeTypeDigest,
  PasswordFormSchemeTypeOther,
  PasswordFormSchemeMobile,
  PasswordFormSchemeUsernameOnly
};

@protocol PasswordStoreObserver;
@protocol PasswordStoreListener;

NS_SWIFT_NAME(PasswordForm)
OBJC_EXPORT
@interface IOSPasswordForm : NSObject <NSCopying>

@property(nonatomic, strong) NSURL* url;
@property(nonatomic, strong) NSString* signOnRealm;
@property(nonatomic, nullable, copy) NSDate* dateCreated;
@property(nonatomic, nullable, copy) NSDate* dateLastUsed;
@property(nonatomic, nullable, copy) NSDate* datePasswordChanged;
@property(nonatomic, nullable, copy) NSString* usernameElement;
@property(nonatomic, nullable, copy) NSString* usernameValue;
@property(nonatomic, nullable, copy) NSString* passwordElement;
@property(nonatomic, nullable, copy) NSString* passwordValue;
@property(nonatomic) bool isBlockedByUser;
@property(nonatomic) PasswordFormScheme scheme;

/// Password Form Constructor used with BravePasswordAPI
/// @param url - Primary data used by the PasswordManager to decide (in longest
///        matching prefix fashion) whether or not a given PasswordForm
///        result from the database is a good fit for a particular
///        form on a page
/// @param signOnRealm - The "Realm" for the sign-on. This is scheme, host,
///        port for SCHEME_HTML
///        The signon_realm is effectively the primary key used for retrieving
///        data from the database, so it must not be empty
/// @param dateCreated - The date when login was saved
/// @param dateLastUsed - The date when the login was last used by the user
/// to login to the site
/// @param datePasswordChanged - Date when the password value was last changed
/// @param usernameElement - The string representation of the name of the
/// username input element
/// @param usernameValue - The string representation of the username
/// @param passwordElement - The string representation of the name of the
/// password input element
/// @param passwordValue - The string representation of the password
- (instancetype)initWithURL:(NSURL*)url
                signOnRealm:(NSString*)signOnRealm
                dateCreated:(nullable NSDate*)dateCreated
               dateLastUsed:(nullable NSDate*)dateLastUsed
        datePasswordChanged:(nullable NSDate*)datePasswordChanged
            usernameElement:(nullable NSString*)usernameElement
              usernameValue:(nullable NSString*)usernameValue
            passwordElement:(nullable NSString*)passwordElement
              passwordValue:(nullable NSString*)passwordValue
            isBlockedByUser:(bool)isBlockedByUser
                     scheme:(PasswordFormScheme)scheme;

- (void)updatePasswordForm:(nullable NSString*)usernameValue
             passwordValue:(nullable NSString*)passwordValue;

@end

NS_SWIFT_NAME(BravePasswordAPI)
OBJC_EXPORT
@interface BravePasswordAPI : NSObject

@property(nonatomic, readonly) bool isAbleToSavePasswords;

- (id<PasswordStoreListener>)addObserver:(id<PasswordStoreObserver>)observer;
- (void)removeObserver:(id<PasswordStoreListener>)observer;

- (instancetype)init NS_UNAVAILABLE;

/// Add Login Method that add form using password store
/// @param passwordForm - Password Form to be added
- (void)addLogin:(IOSPasswordForm*)passwordForm;

/// Remove Selected Password Forms
/// @param passwordForm - Password Form to be removed from the store
- (void)removeLogin:(IOSPasswordForm*)passwordForm;

/// Update List of Password Form
/// @param newPasswordForm - Updated Password Form including primary keys
/// @param oldPasswordForm - Old PasswordForm to be updated
- (void)updateLogin:(IOSPasswordForm*)newPasswordForm
    oldPasswordForm:(IOSPasswordForm*)oldPasswordForm;

/// Fetch Function that will return list of all Password Forms
/// from saved password presenter
/// @param completion - Block that notifies querying is finished with list of
/// credentials
- (void)getSavedLogins:(void (^)(NSArray<IOSPasswordForm*>*))completion;

/// Fetch Function that will return list of all Password Forms
/// from saved password presenter
/// @param url - URL of the Password Forms that are queried
/// @param formScheme - Form Scheme of the Password Form
/// @param completion - Block that notifies querying is finished with list of
/// credentials
- (void)getSavedLoginsForURL:(NSURL*)url
                  formScheme:(PasswordFormScheme)formScheme
                  completion:(void (^)(NSArray<IOSPasswordForm*>*))completion;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_PASSWORD_BRAVE_PASSWORD_API_H_
