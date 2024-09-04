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
@property(nonatomic, readonly, nullable) NSString* filter;
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

/// Initialize an adblock engine with a set of serialized rules. Returns
/// nil/Throws an error if the engine cannot decode the rules
- (nullable instancetype)initWithSerializedData:(NSData*)data
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

/// Deserializes a previously serialized data file list.
- (bool)deserialize:(NSData*)data NS_SWIFT_NAME(deserialize(data:));

/// Serializes the engine to a data file list.
- (nullable NSData*)serialize:(NSError**)error;

/// Adds a tag to the engine for consideration
- (void)addTag:(NSString*)tag;

/// Removes a tag to the engine for consideration
- (void)removeTag:(NSString*)tag;

/// Checks if a tag exists in the engine
- (bool)tagExists:(NSString*)tag;

/// Adds a resource to the engine by name
// BOOL return type must be used so Swift turns this into a throwing function
- (BOOL)addResourceWithKey:(NSString*)key
               contentType:(NSString*)contentType
                      data:(NSString*)data
                     error:(NSError**)error
    NS_SWIFT_NAME(addResource(key:contentType:data:));

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
