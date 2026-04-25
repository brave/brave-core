/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_MAC_KEYSTONE_REGISTRATION_H_
#define BRAVE_BROWSER_MAC_KEYSTONE_REGISTRATION_H_

#import <Foundation/Foundation.h>
#include <Security/Authorization.h>
#include <stdint.h>

// Declarations of the Keystone registration bits needed here. From
// KSRegistration.h.

namespace keystone_registration {

typedef enum {
  kKSPathExistenceChecker,
} KSExistenceCheckerType;

typedef enum {
  kKSRegistrationUserTicket,
  kKSRegistrationSystemTicket,
  kKSRegistrationDontKnowWhatKindOfTicket,
} KSRegistrationTicketType;

extern NSString* KSRegistrationVersionKey;
extern NSString* KSRegistrationExistenceCheckerTypeKey;
extern NSString* KSRegistrationExistenceCheckerStringKey;
extern NSString* KSRegistrationServerURLStringKey;
extern NSString* KSRegistrationPreserveTrustedTesterTokenKey;
extern NSString* KSRegistrationTagKey;
extern NSString* KSRegistrationTagPathKey;
extern NSString* KSRegistrationTagKeyKey;
extern NSString* KSRegistrationBrandPathKey;
extern NSString* KSRegistrationBrandKeyKey;
extern NSString* KSRegistrationVersionPathKey;
extern NSString* KSRegistrationVersionKeyKey;

extern NSString* KSRegistrationDidCompleteNotification;
extern NSString* KSRegistrationPromotionDidCompleteNotification;

extern NSString* KSRegistrationCheckForUpdateNotification;
extern NSString* KSRegistrationStatusKey;
extern NSString* KSRegistrationUpdateCheckErrorKey;
extern NSString* KSRegistrationUpdateCheckRawResultsKey;
extern NSString* KSRegistrationUpdateCheckRawErrorMessagesKey;

extern NSString* KSRegistrationStartUpdateNotification;
extern NSString* KSUpdateCheckSuccessfulKey;
extern NSString* KSUpdateCheckSuccessfullyInstalledKey;

extern NSString* KSRegistrationRemoveExistingTag;
#define KSRegistrationPreserveExistingTag nil

}  // namespace keystone_registration

@interface KSRegistration : NSObject

+ (instancetype)registrationWithProductID:(NSString*)productID;  // NOLINT

- (BOOL)registerWithParameters:(NSDictionary*)args;  // NOLINT

- (BOOL)promoteWithParameters:(NSDictionary*)args  // NOLINT
                authorization:(AuthorizationRef)authorization;

- (BOOL)setActiveWithError:(NSError**)error;  // NOLINT
- (void)checkForUpdateWasUserInitiated:(BOOL)userInitiated;
- (void)startUpdate;
- (keystone_registration::KSRegistrationTicketType)ticketType;

@end  // @interface KSRegistration

#endif  // BRAVE_BROWSER_MAC_KEYSTONE_REGISTRATION_H_
