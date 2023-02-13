/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/brave_shields/adblock_engine.h"

#include "base/strings/sys_string_conversions.h"
#include "brave/base/mac/conversions.h"
#include "brave/components/brave_shields/adblock/rs/src/lib.rs.h"

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
  auto engine_result = adblock_engine->matches(
      base::SysNSStringToUTF8(url), base::SysNSStringToUTF8(host),
      base::SysNSStringToUTF8(tabHost), base::SysNSStringToUTF8(resourceType),
      isThirdParty, false, false);
  auto result = [[AdblockEngineMatchResult alloc] init];
  result.didMatchRule = engine_result.value.matched;
  result.didMatchException = engine_result.value.has_exception;
  result.didMatchImportant = engine_result.value.important;
  result.redirect = base::SysUTF8ToNSString(engine_result.value.redirect.value);
  result.rewrittenURL =
      base::SysUTF8ToNSString(engine_result.value.rewritten_url.value);
  return result;
}

- (NSString*)cspDirectivesForURL:(NSString*)url
                            host:(NSString*)host
                         tabHost:(NSString*)tabHost
                    isThirdParty:(bool)isThirdParty
                    resourceType:(NSString*)resourceType {
  return base::SysUTF8ToNSString(adblock_engine->get_csp_directives(
      base::SysNSStringToUTF8(url), base::SysNSStringToUTF8(host),
      base::SysNSStringToUTF8(tabHost), base::SysNSStringToUTF8(resourceType),
      isThirdParty));
}

- (bool)deserialize:(NSData*)data {
  auto result = adblock_engine->deserialize(
      static_cast<const char*>(data.bytes), data.length);
  return result.result_kind == adblock::ResultKind::Success;
}

- (void)addTag:(NSString*)tag {
  adblock_engine->enable_tag(base::SysNSStringToUTF8(tag));
}

- (void)removeTag:(NSString*)tag {
  adblock_engine->disable_tag(base::SysNSStringToUTF8(tag));
}

- (bool)tagExists:(NSString*)tag {
  return adblock_engine->tag_exists(base::SysNSStringToUTF8(tag)).value;
}

- (void)addResourceWithKey:(NSString*)key
               contentType:(NSString*)contentType
                      data:(NSString*)data {
  adblock_engine->add_resource(base::SysNSStringToUTF8(key),
                               base::SysNSStringToUTF8(contentType),
                               base::SysNSStringToUTF8(data));
}

- (void)useResources:(NSString*)resources {
  adblock_engine->use_resources(base::SysNSStringToUTF8(resources));
}

- (NSString*)cosmeticResourcesForURL:(NSString*)url {
  return base::SysUTF8ToNSString(
      adblock_engine->url_cosmetic_resources(base::SysNSStringToUTF8(url))
          .value);
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

+ (bool)setDomainResolver {
  return adblock::set_domain_resolver();

  +(NSString*)contentBlockerRulesFromFilterSet : (NSString*)filterSet truncated
      : (bool*)truncated {
    return base::SysUTF8ToNSString(adblock::ConvertRulesToContentBlockingRules(
        base::SysNSStringToUTF8(filterSet), truncated));
  }

  @end
