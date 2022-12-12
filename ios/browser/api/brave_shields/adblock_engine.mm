/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/brave_shields/adblock_engine.h"

#include "base/strings/sys_string_conversions.h"
#include "brave/base/mac/conversions.h"
#include "brave/components/adblock_rust_ffi/src/wrapper.h"
#include "brave/components/brave_shields/common/adblock_domain_resolver.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface AdblockEngineMatchResult ()
@property(nonatomic, readwrite) bool didMatchRule;
@property(nonatomic, readwrite) bool didMatchException;
@property(nonatomic, readwrite) bool didMatchImportant;
@property(nonatomic, readwrite, copy) NSString* redirect;
@property(nonatomic, readwrite, copy) NSString* rewrittenURL;
@end

@implementation AdblockEngineMatchResult
- (instancetype)init {
  if ((self = [super init])) {
    self.redirect = @"";
    self.rewrittenURL = @"";
  }
  return self;
}
@end

@implementation AdblockEngine {
  std::unique_ptr<adblock::Engine> adblock_engine;
}

- (instancetype)init {
  return [self initWithRules:@""];
}

- (instancetype)initWithRules:(NSString*)rules {
  if ((self = [super init])) {
    adblock_engine =
        std::make_unique<adblock::Engine>(base::SysNSStringToUTF8(rules));
  }
  return self;
}

- (AdblockEngineMatchResult*)matchesURL:(NSString*)url
                                   host:(NSString*)host
                                tabHost:(NSString*)tabHost
                           isThirdParty:(bool)isThirdParty
                           resourceType:(NSString*)resourceType {
  bool did_match_rule = false;
  bool did_match_exception = false;
  bool did_match_important = false;
  std::string redirect = "";
  std::string rewritten_url = "";
  adblock_engine->matches(
      base::SysNSStringToUTF8(url), base::SysNSStringToUTF8(host),
      base::SysNSStringToUTF8(tabHost), isThirdParty,
      base::SysNSStringToUTF8(resourceType), &did_match_rule,
      &did_match_exception, &did_match_important, &redirect, &rewritten_url);
  auto result = [[AdblockEngineMatchResult alloc] init];
  result.didMatchRule = did_match_rule;
  result.didMatchException = did_match_exception;
  result.didMatchImportant = did_match_important;
  result.redirect = base::SysUTF8ToNSString(redirect);
  result.rewrittenURL = base::SysUTF8ToNSString(rewritten_url);
  return result;
}

- (NSString*)cspDirectivesForURL:(NSString*)url
                            host:(NSString*)host
                         tabHost:(NSString*)tabHost
                    isThirdParty:(bool)isThirdParty
                    resourceType:(NSString*)resourceType {
  return base::SysUTF8ToNSString(adblock_engine->getCspDirectives(
      base::SysNSStringToUTF8(url), base::SysNSStringToUTF8(host),
      base::SysNSStringToUTF8(tabHost), isThirdParty,
      base::SysNSStringToUTF8(resourceType)));
}

- (bool)deserialize:(NSData*)data {
  return adblock_engine->deserialize(static_cast<const char*>(data.bytes),
                                     data.length);
}

- (void)addTag:(NSString*)tag {
  adblock_engine->addTag(base::SysNSStringToUTF8(tag));
}

- (void)removeTag:(NSString*)tag {
  adblock_engine->removeTag(base::SysNSStringToUTF8(tag));
}

- (bool)tagExists:(NSString*)tag {
  return adblock_engine->tagExists(base::SysNSStringToUTF8(tag));
}

- (void)addResourceWithKey:(NSString*)key
               contentType:(NSString*)contentType
                      data:(NSString*)data {
  adblock_engine->addResource(base::SysNSStringToUTF8(key),
                              base::SysNSStringToUTF8(contentType),
                              base::SysNSStringToUTF8(data));
}

- (void)useResources:(NSString*)resources {
  adblock_engine->useResources(base::SysNSStringToUTF8(resources));
}

- (NSString*)cosmeticResourcesForURL:(NSString*)url {
  return base::SysUTF8ToNSString(
      adblock_engine->urlCosmeticResources(base::SysNSStringToUTF8(url)));
}

- (NSString*)
    stylesheetForCosmeticRulesIncludingClasses:(NSArray<NSString*>*)classes
                                           ids:(NSArray<NSString*>*)ids
                                    exceptions:(NSArray<NSString*>*)exceptions {
  return base::SysUTF8ToNSString(adblock_engine->hiddenClassIdSelectors(
      brave::ns_to_vector<std::string>(classes),
      brave::ns_to_vector<std::string>(ids),
      brave::ns_to_vector<std::string>(exceptions)));
}

+ (bool)setDomainResolver:(DomainResolverCallback)domainResolver {
  return adblock::SetDomainResolver(domainResolver);
}

+ (DomainResolverCallback)defaultDomainResolver {
  return brave_shields::AdBlockServiceDomainResolver;
}

+ (NSString*)contentBlockerRulesFromFilterSet:(NSString*)filterSet {
  return base::SysUTF8ToNSString(adblock::ConvertRulesToContentBlockingRules(
      base::SysNSStringToUTF8(filterSet)));
}

@end
