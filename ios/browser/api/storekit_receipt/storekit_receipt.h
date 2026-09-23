/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_STOREKIT_RECEIPT_STOREKIT_RECEIPT_H_
#define BRAVE_IOS_BROWSER_API_STOREKIT_RECEIPT_STOREKIT_RECEIPT_H_

#import <Foundation/Foundation.h>
#import <Security/Security.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface BraveStoreKitPurchase : NSObject
@property(nonatomic, readonly) NSUInteger quantity;
@property(nonatomic, readonly) NSString* productId;
@property(nonatomic, readonly) NSString* transactionId;
@property(nonatomic, readonly) NSString* originalTransactionId;
@property(nonatomic, readonly, nullable) NSDate* purchaseDate;
@property(nonatomic, readonly, nullable) NSDate* originalPurchaseDate;
@property(nonatomic, readonly, nullable) NSDate* subscriptionExpirationDate;
@property(nonatomic, readonly, nullable) NSDate* cancellationDate;
@property(nonatomic, readonly) NSUInteger webOrderLineItemId;
@property(nonatomic, readonly) bool isInIntroOfferPeriod;

- (instancetype)init NS_UNAVAILABLE;
@end

OBJC_EXPORT
@interface BraveStoreKitReceipt : NSObject
@property(nonatomic, readonly) NSString* bundleId;
@property(nonatomic, readonly) NSString* appVersion;
@property(nonatomic, readonly, nullable) NSData* opaqueData;
@property(nonatomic, readonly) NSString* sha1Hash;
@property(nonatomic, readonly)
    NSArray<BraveStoreKitPurchase*>* inAppPurchaseReceipts;
@property(nonatomic, readonly) NSString* originalApplicationVersion;
@property(nonatomic, readonly, nullable) NSDate* receiptCreationDate;
@property(nonatomic, readonly, nullable) NSDate* receiptExpirationDate;

- (instancetype)init NS_UNAVAILABLE;
- (nullable instancetype)initWithData:(nonnull NSData*)data
    NS_DESIGNATED_INITIALIZER;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_STOREKIT_RECEIPT_STOREKIT_RECEIPT_H_
