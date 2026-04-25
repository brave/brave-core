/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import "brave/browser/mac/keystone_registration.h"

namespace keystone_registration {

// Definitions of the Keystone registration constants needed here. From
// KSRegistration.m.

NSString* KSRegistrationVersionKey = @"Version";
NSString* KSRegistrationExistenceCheckerTypeKey = @"ExistenceCheckerType";
NSString* KSRegistrationExistenceCheckerStringKey = @"ExistenceCheckerString";
NSString* KSRegistrationServerURLStringKey = @"URLString";
NSString* KSRegistrationPreserveTrustedTesterTokenKey = @"PreserveTTT";
NSString* KSRegistrationTagKey = @"Tag";
NSString* KSRegistrationTagPathKey = @"TagPath";
NSString* KSRegistrationTagKeyKey = @"TagKey";
NSString* KSRegistrationBrandPathKey = @"BrandPath";
NSString* KSRegistrationBrandKeyKey = @"BrandKey";
NSString* KSRegistrationVersionPathKey = @"VersionPath";
NSString* KSRegistrationVersionKeyKey = @"VersionKey";

NSString* KSRegistrationDidCompleteNotification =
    @"KSRegistrationDidCompleteNotification";
NSString* KSRegistrationPromotionDidCompleteNotification =
    @"KSRegistrationPromotionDidCompleteNotification";

NSString* KSRegistrationCheckForUpdateNotification =
    @"KSRegistrationCheckForUpdateNotification";
NSString* KSRegistrationStatusKey = @"Status";
NSString* KSRegistrationUpdateCheckErrorKey = @"Error";
NSString* KSRegistrationUpdateCheckRawResultsKey = @"RawResults";
NSString* KSRegistrationUpdateCheckRawErrorMessagesKey = @"RawErrorMessages";

NSString* KSRegistrationStartUpdateNotification =
    @"KSRegistrationStartUpdateNotification";
NSString* KSUpdateCheckSuccessfulKey = @"CheckSuccessful";
NSString* KSUpdateCheckSuccessfullyInstalledKey = @"SuccessfullyInstalled";

NSString* KSRegistrationRemoveExistingTag = @"";

NSString* KSReportingAttributeValueKey = @"value";
NSString* KSReportingAttributeExpirationDateKey = @"expiresAt";
NSString* KSReportingAttributeAggregationTypeKey = @"aggregation";

}  // namespace keystone_registration
