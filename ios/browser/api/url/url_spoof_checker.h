/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_URL_URL_SPOOF_CHECKER_H_
#define BRAVE_IOS_BROWSER_API_URL_URL_SPOOF_CHECKER_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/// The |SkeletonType| and |TopDomainEntry| are mirrored in trie_entry.h. These
/// are used to insert and read nodes from the Trie.
/// The type of skeleton in the trie node.
typedef NSInteger BraveSpoofCheckerSkeletonType NS_TYPED_ENUM
    NS_SWIFT_NAME(URLSpoofChecker.SkeletonType);
OBJC_EXPORT BraveSpoofCheckerSkeletonType const
    BraveSpoofCheckerSkeletonTypeFull;
OBJC_EXPORT BraveSpoofCheckerSkeletonType const
    BraveSpoofCheckerSkeletonTypeSeparatorsRemoved;

typedef NSInteger BraveSpoofCheckerLookalikeURLMatchType NS_TYPED_ENUM
    NS_SWIFT_NAME(URLSpoofChecker.LookalikeURLMatchType);

OBJC_EXPORT BraveSpoofCheckerLookalikeURLMatchType const
    BraveSpoofCheckerLookalikeURLMatchTypeNone;
OBJC_EXPORT BraveSpoofCheckerLookalikeURLMatchType const
    BraveSpoofCheckerLookalikeURLMatchTypeSkeletonMatchSiteEngagement;
OBJC_EXPORT BraveSpoofCheckerLookalikeURLMatchType const
    BraveSpoofCheckerLookalikeURLMatchTypeEditDistance;
OBJC_EXPORT BraveSpoofCheckerLookalikeURLMatchType const
    BraveSpoofCheckerLookalikeURLMatchTypeEditDistanceSiteEngagement;
OBJC_EXPORT BraveSpoofCheckerLookalikeURLMatchType const
    BraveSpoofCheckerLookalikeURLMatchTypeTargetEmbedding;
OBJC_EXPORT BraveSpoofCheckerLookalikeURLMatchType const
    BraveSpoofCheckerLookalikeURLMatchTypeSkeletonMatchTop500;
OBJC_EXPORT BraveSpoofCheckerLookalikeURLMatchType const
    BraveSpoofCheckerLookalikeURLMatchTypeSkeletonMatchTop5k;
OBJC_EXPORT BraveSpoofCheckerLookalikeURLMatchType const
    BraveSpoofCheckerLookalikeURLMatchTypeTargetEmbeddingForSafetyTips;
/// The domain name failed IDN spoof checks but didn't match a safe hostname.
/// As a result, there is no URL to suggest to the user in the form of "Did
/// you mean <url>?".
OBJC_EXPORT BraveSpoofCheckerLookalikeURLMatchType const
    BraveSpoofCheckerLookalikeURLMatchTypeFailedSpoofChecks;
OBJC_EXPORT BraveSpoofCheckerLookalikeURLMatchType const
    BraveSpoofCheckerLookalikeURLMatchTypeCharacterSwapSiteEngagement;
OBJC_EXPORT BraveSpoofCheckerLookalikeURLMatchType const
    BraveSpoofCheckerLookalikeURLMatchTypeCharacterSwapTop500;

OBJC_EXPORT
NS_SWIFT_NAME(URLSpoofChecker.TopDomainEntry)
@interface URLSpoofCheckerTopDomainEntry : NSObject
- (instancetype)init NS_UNAVAILABLE;
/// The domain name.
@property(nonatomic, readonly) NSString* domain;
/// True if the domain is in the top 500.
@property(nonatomic, readonly) bool isTop500;
/// Type of the skeleton stored in the trie node.
@property(nonatomic, readonly) BraveSpoofCheckerSkeletonType skeletonType;
@end

OBJC_EXPORT
NS_SWIFT_NAME(URLSpoofChecker.Result)
@interface BraveURLSpoofCheckerResult : NSObject
- (instancetype)init NS_UNAVAILABLE;
@property(nonatomic, readonly)
    BraveSpoofCheckerLookalikeURLMatchType urlMatchType;
@property(nonatomic, readonly, nullable) NSURL* suggestedURL;
@end

OBJC_EXPORT
NS_SWIFT_NAME(URLSpoofChecker)
@interface BraveURLSpoofChecker : NSObject
- (instancetype)init NS_UNAVAILABLE;
+ (URLSpoofCheckerTopDomainEntry*)getSimilarTopDomain:(NSString*)hostname;
+ (URLSpoofCheckerTopDomainEntry*)lookupSkeletonInTopDomains:
    (NSString*)hostname;
+ (NSArray<NSString*>*)getSkeletons:(NSString*)url;
+ (BraveURLSpoofCheckerResult*)isLookalikeURL:(NSString*)url;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_URL_URL_SPOOF_CHECKER_H_
