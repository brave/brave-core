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
@property(nonatomic, readwrite, copy) NSString* filter;
@end

@interface ContentBlockingRulesResult ()
@property(nonatomic, readwrite, copy) NSString* rulesJSON;
@property(nonatomic, readwrite) bool truncated;
@end

@implementation AdblockEngineMatchResult
- (instancetype)init {
  if ((self = [super init])) {
    self.redirect = @"";
    self.rewrittenURL = @"";
    self.filter = @"";
  }
  return self;
}
@end

@implementation ContentBlockingRulesResult
- (instancetype)init {
  if ((self = [super init])) {
    self.rulesJSON = @"";
  }
  return self;
}
@end

/// rust::Box's default constructor is deleted, so we must box it again so we
/// can assign it `adblock::new_engine()` by default since C++ types inside of
/// Obj-C built with ARC call their default constructor on `-init` regardless
class AdblockEngineBox final {
 public:
  AdblockEngineBox() : adblock_engine_(adblock::new_engine()) {}
  AdblockEngineBox(const AdblockEngineBox&) = delete;
  AdblockEngineBox& operator=(const AdblockEngineBox&) = delete;
  ~AdblockEngineBox() = default;

  rust::Box<adblock::Engine>& operator->() { return adblock_engine_; }
  void operator=(rust::Box<adblock::Engine>&& engine) {
    adblock_engine_ = std::move(engine);
  }

 private:
  rust::Box<adblock::Engine> adblock_engine_;
};

@implementation AdblockEngine {
  AdblockEngineBox adblock_engine;
}

- (instancetype)init {
  // An empty engine is already created with `AdblockEngineBox`
  return [super init];
}

- (instancetype)initWithRules:(NSString*)rules error:(NSError**)error {
  if ((self = [super init])) {
    if (rules.length > 0) {
      std::vector<std::uint8_t> vecRules;
      NSData* data = [rules dataUsingEncoding:NSUTF8StringEncoding];

      if (data) {
        vecRules.resize(data.length);
        [data getBytes:vecRules.data() length:data.length];
      }

      auto result = adblock::engine_with_rules(vecRules);
      if (result.result_kind == adblock::ResultKind::Success) {
        adblock_engine = std::move(result.value);
      } else {
        if (error) {
          *error = [[self class] adblockErrorForKind:result.result_kind
                                             message:result.error_message];
        }
      }
    }
  }
  return self;
}

- (instancetype)initWithSerializedData:(NSData*)data error:(NSError**)error {
  if ((self = [super init])) {
    if (![self deserialize:data]) {
      if (error) {
        *error =
            [[self class] adblockErrorForKind:adblock::ResultKind::AdblockError
                                      message:"Failed to deserialize data"];
      }
    }
  }
  return self;
}

+ (NSError*)adblockErrorForKind:(adblock::ResultKind)kind
                        message:(rust::String)message {
  return [NSError
      errorWithDomain:@"com.brave.adblock"
                 code:static_cast<NSInteger>(kind)
             userInfo:@{
               NSLocalizedDescriptionKey :
                   base::SysUTF8ToNSString(static_cast<std::string>(message))
             }];
}

- (AdblockEngineMatchResult*)matchesURL:(NSString*)url
                                   host:(NSString*)host
                                tabHost:(NSString*)tabHost
                           isThirdParty:(bool)isThirdParty
                           resourceType:(NSString*)resourceType {
  return [self matchesURL:url
                       host:host
                    tabHost:tabHost
               isThirdParty:isThirdParty
               resourceType:resourceType
      previouslyMatchedRule:false
       forceCheckExceptions:false];
}

- (AdblockEngineMatchResult*)matchesURL:(NSString*)url
                                   host:(NSString*)host
                                tabHost:(NSString*)tabHost
                           isThirdParty:(bool)isThirdParty
                           resourceType:(NSString*)resourceType
                  previouslyMatchedRule:(bool)previouslyMatchedRule
                   forceCheckExceptions:(bool)forceCheckExceptions {
  auto engine_result = adblock_engine->matches(
      base::SysNSStringToUTF8(url), base::SysNSStringToUTF8(host),
      base::SysNSStringToUTF8(tabHost), base::SysNSStringToUTF8(resourceType),
      isThirdParty, previouslyMatchedRule, forceCheckExceptions);
  auto result = [[AdblockEngineMatchResult alloc] init];
  result.didMatchRule = engine_result.matched;
  result.didMatchException = engine_result.has_exception;
  result.didMatchImportant = engine_result.important;
  if (engine_result.redirect.has_value) {
    result.redirect = base::SysUTF8ToNSString(
        static_cast<std::string>(engine_result.redirect.value));
  }
  if (engine_result.rewritten_url.has_value) {
    result.rewrittenURL = base::SysUTF8ToNSString(
        static_cast<std::string>(engine_result.rewritten_url.value));
  }
  if (engine_result.filter.has_value) {
    result.filter = base::SysUTF8ToNSString(
        static_cast<std::string>(engine_result.filter.value));
  }
  return result;
}

- (NSString*)cspDirectivesForURL:(NSString*)url
                            host:(NSString*)host
                         tabHost:(NSString*)tabHost
                    isThirdParty:(bool)isThirdParty
                    resourceType:(NSString*)resourceType {
  return base::SysUTF8ToNSString(
      static_cast<std::string>(adblock_engine->get_csp_directives(
          base::SysNSStringToUTF8(url), base::SysNSStringToUTF8(host),
          base::SysNSStringToUTF8(tabHost),
          base::SysNSStringToUTF8(resourceType), isThirdParty)));
}

- (bool)deserialize:(NSData*)data {
  std::vector<std::uint8_t> vecData(data.length);
  [data getBytes:vecData.data() length:data.length];
  return adblock_engine->deserialize(vecData);
}

- (nullable NSData*)serialize:(NSError**)error {
  auto result = adblock_engine->serialize();

  if (result.empty()) {
    if (error) {
      *error =
          [[self class] adblockErrorForKind:adblock::ResultKind::AdblockError
                                    message:"Failed to serialize data"];
    }
    return nil;
  }

  return [NSData dataWithBytes:result.data() length:result.size()];
}

- (void)addTag:(NSString*)tag {
  adblock_engine->enable_tag(base::SysNSStringToUTF8(tag));
}

- (void)removeTag:(NSString*)tag {
  adblock_engine->disable_tag(base::SysNSStringToUTF8(tag));
}

- (bool)tagExists:(NSString*)tag {
  return adblock_engine->tag_exists(base::SysNSStringToUTF8(tag));
}

- (bool)addResourceWithKey:(NSString*)key
               contentType:(NSString*)contentType
                      data:(NSString*)data
                     error:(NSError**)error {
  const auto result = adblock_engine->add_resource(
      base::SysNSStringToUTF8(key), base::SysNSStringToUTF8(contentType),
      base::SysNSStringToUTF8(data));
  const auto success = result.result_kind == adblock::ResultKind::Success;
  if (result.result_kind != adblock::ResultKind::Success && error) {
    *error = [[self class] adblockErrorForKind:result.result_kind
                                       message:result.error_message];
  }
  return success;
}

- (bool)useResources:(NSString*)resources {
  return adblock_engine->use_resources(base::SysNSStringToUTF8(resources));
}

- (NSString*)cosmeticResourcesForURL:(NSString*)url {
  return base::SysUTF8ToNSString(static_cast<std::string>(
      adblock_engine->url_cosmetic_resources(base::SysNSStringToUTF8(url))));
}

- (nullable NSArray<NSString*>*)
    stylesheetForCosmeticRulesIncludingClasses:(NSArray<NSString*>*)classes
                                           ids:(NSArray<NSString*>*)ids
                                    exceptions:(NSArray<NSString*>*)exceptions
                                         error:(NSError**)error {
  const auto result = adblock_engine->hidden_class_id_selectors(
      brave::ns_to_vector<std::string>(classes),
      brave::ns_to_vector<std::string>(ids),
      brave::ns_to_vector<std::string>(exceptions));
  if (result.result_kind != adblock::ResultKind::Success) {
    if (error) {
      *error = [[self class] adblockErrorForKind:result.result_kind
                                         message:result.error_message];
    }
    return nil;
  }
  auto selectors = [[NSMutableArray<NSString*> alloc] init];
  for (auto selector : result.value) {
    [selectors
        addObject:base::SysUTF8ToNSString(static_cast<std::string>(selector))];
  }
  return [selectors copy];
}

+ (bool)setDomainResolver {
  return adblock::set_domain_resolver();
}

+ (nullable ContentBlockingRulesResult*)
    contentBlockerRulesFromFilterSet:(NSString*)filterSet
                               error:(NSError**)error {
  auto result = adblock::convert_rules_to_content_blocking(
      base::SysNSStringToUTF8(filterSet));
  if (result.result_kind != adblock::ResultKind::Success) {
    if (error) {
      *error = [self adblockErrorForKind:result.result_kind
                                 message:result.error_message];
    }
    return nil;
  }
  auto value = [[ContentBlockingRulesResult alloc] init];
  value.rulesJSON = base::SysUTF8ToNSString(
      static_cast<std::string>(result.value.rules_json));
  value.truncated = result.value.truncated;
  return value;
}

@end
