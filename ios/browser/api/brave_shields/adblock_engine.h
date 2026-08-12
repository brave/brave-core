/* Copyright (c) 2022 The Brave Authors. All rights reserved.
/// This Source Code Form is subject to the terms of the Mozilla Public
/// License, v. 2.0. If a copy of the MPL was not distributed with this file,
/// You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_BRAVE_SHIELDS_ADBLOCK_ENGINE_H_
#define BRAVE_IOS_BROWSER_API_BRAVE_SHIELDS_ADBLOCK_ENGINE_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
NS_SWIFT_NAME(AdblockEngine.MatchResult)
@interface AdblockEngineMatchResult : NSObject
@property(nonatomic, readonly) bool didMatchRule;
@property(nonatomic, readonly) bool didMatchException;
@property(nonatomic, readonly) bool didMatchImportant;
@property(nonatomic, readonly, nullable) NSString* redirect;
@property(nonatomic, readonly, nullable) NSString* rewrittenURL;
@end

OBJC_EXPORT
@interface ContentBlockingRulesResult : NSObject
@property(nonatomic, readonly) NSString* rulesJSON;
@property(nonatomic, readonly) bool truncated;
@end

OBJC_EXPORT
@interface AdblockEngine : NSObject

/// Initialize an empty adblock engine
- (instancetype)init;

/// Initialize an adblock engine with a set of rules. Returns nil/Throws an
/// error if the engine cannot parse the rules provided due to invalid UTF8
- (nullable instancetype)initWithRules:(NSString*)rules error:(NSError**)error;

/// Initialize an adblock engine with a set of rules read from a file. Returns
/// nil/Throws an error if the file cannot be read, or if the engine cannot
/// parse the rules provided due to invalid UTF8.
///
/// Prefer this over ``initWithRules:error:`` for large filter lists: the file
/// is read into a single buffer instead of being copied through `NSString`
/// and `NSData` first.
- (nullable instancetype)initWithRulesFileURL:(NSURL*)fileURL
                                        error:(NSError**)error;

/// Initialize an adblock engine with a set of serialized rules read from a
/// file. Returns nil/Throws an error if the file cannot be read or if the
/// engine cannot decode the rules.
- (nullable instancetype)initWithSerializedFileURL:(NSURL*)fileURL
                                             error:(NSError**)error;

/// Checks if a `url` matches for the specified `Engine` within the context.
///
/// This API is designed for multi-engine use, so block results are used both as
/// inputs and outputs. They will be updated to reflect additional checking
/// within this engine, rather than being replaced with results just for this
/// engine.
- (AdblockEngineMatchResult*)matchesURL:(NSString*)url
                                   host:(NSString*)host
                                tabHost:(NSString*)tabHost
                           isThirdParty:(bool)isThirdParty
                           resourceType:(NSString*)resourceType
    NS_SWIFT_NAME(matches(url:host:tabHost:isThirdParty:resourceType:));

/// Checks if a `url` matches for the specified `Engine` within the context.
///
/// This API is designed for multi-engine use, so block results are used both as
/// inputs and outputs. They will be updated to reflect additional checking
/// within this engine, rather than being replaced with results just for this
/// engine.
- (AdblockEngineMatchResult*)matchesURL:(NSString*)url
                                   host:(NSString*)host
                                tabHost:(NSString*)tabHost
                           isThirdParty:(bool)isThirdParty
                           resourceType:(NSString*)resourceType
                  previouslyMatchedRule:(bool)previouslyMatchedRule
                   forceCheckExceptions:(bool)forceCheckExceptions
    NS_SWIFT_NAME(matches(url:host:tabHost:isThirdParty:resourceType:previouslyMatchedRule:forceCheckExceptions:));

/// Returns any CSP directives that should be added to a subdocument or document
/// request's response headers.
- (NSString*)cspDirectivesForURL:(NSString*)url
                            host:(NSString*)host
                         tabHost:(NSString*)tabHost
                    isThirdParty:(bool)isThirdParty
                    resourceType:(NSString*)resourceType;

/// Serializes the engine to a data file list at the given file URL, replacing
/// any file already at that location.
///
/// Writing directly to disk avoids holding a second copy of the serialized
/// engine in memory, which can be substantial for large filter lists.
- (BOOL)serializeToFileURL:(NSURL*)fileURL
                     error:(NSError**)error NS_SWIFT_NAME(serialize(to:));

/// Uses a list of `Resource`s from JSON format
- (bool)useResources:(NSString*)resources;

/// Returns a set of cosmetic filtering resources specific to the given url, in
/// JSON format
- (NSString*)cosmeticResourcesForURL:(NSString*)url
    NS_SWIFT_NAME(cosmeticResourcesForURL(_:));

/// Returns list of CSS selectors that require a generic CSS hide rule,
/// from a given set of classes, ids and exceptions
///
/// The leading '.' or '#' character should not be provided
- (nullable NSArray<NSString*>*)
    stylesheetForCosmeticRulesIncludingClasses:(NSArray<NSString*>*)classes
                                           ids:(NSArray<NSString*>*)ids
                                    exceptions:(NSArray<NSString*>*)exceptions
                                         error:(NSError**)error
    NS_SWIFT_NAME(stylesheetForCosmeticRulesIncluding(classes:ids:exceptions:));

/// Sets the domain resolver
/// This is required to be able to use any adblocking functionality.
///
/// Returns true on success, false if it was already set previously.
+ (bool)setDomainResolver;

/// Converts ABP rules/filter sets into Content Blocker rules that can be used
/// with ``WKWebView``
+ (nullable ContentBlockingRulesResult*)
    contentBlockerRulesFromFilterSet:(NSString*)filterSet
                               error:(NSError**)error;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_BRAVE_SHIELDS_ADBLOCK_ENGINE_H_
