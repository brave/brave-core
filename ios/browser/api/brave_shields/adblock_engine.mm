/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/brave_shields/adblock_engine.h"

#include <optional>
#include <string>
#include <vector>

#include "base/apple/foundation_util.h"
#include "base/compiler_specific.h"
#include "base/containers/span.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/base/apple/foundation_util.h"
#include "brave/components/brave_shields/core/common/adblock/rs/src/lib.rs.h"

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

@interface ContentBlockingRulesResult ()
@property(nonatomic, readwrite, copy) NSString* rulesJSON;
@property(nonatomic, readwrite) bool truncated;
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

namespace {

/// Reads the contents of the file at `path` into a single buffer sized to the
/// file. Filter lists are large enough that reading them through `NSData` or
/// `NSString` first, only to copy them again for the engine, is a meaningful
/// amount of memory.
std::optional<std::vector<std::uint8_t>> ReadFileBytes(
    const base::FilePath& path) {
  base::File file(path, base::File::FLAG_OPEN | base::File::FLAG_READ);
  if (!file.IsValid()) {
    return std::nullopt;
  }

  const int64_t length = file.GetLength();
  if (length < 0 || !base::IsValueInRangeForNumericType<size_t>(length)) {
    return std::nullopt;
  }

  std::vector<std::uint8_t> bytes(static_cast<size_t>(length));
  if (!file.ReadAndCheck(0, bytes)) {
    return std::nullopt;
  }
  return bytes;
}

}  // namespace

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
      const std::string utf8Rules = base::SysNSStringToUTF8(rules);
      const auto utf8Bytes = base::as_byte_span(utf8Rules);
      const std::vector<std::uint8_t> vecRules(utf8Bytes.begin(),
                                               utf8Bytes.end());
      auto result = adblock::engine_with_rules(vecRules);
      if (![self setEngineFromResult:std::move(result) error:error]) {
        return nil;
      }
    }
  }
  return self;
}

- (instancetype)initWithRulesFileURL:(NSURL*)fileURL error:(NSError**)error {
  if ((self = [super init])) {
    // The rules are read by the engine itself so that the file contents are
    // never copied between being read and being parsed.
    auto result = adblock::engine_with_rules_from_file(
        base::apple::NSURLToFilePath(fileURL).value());
    if (![self setEngineFromResult:std::move(result) error:error]) {
      return nil;
    }
  }
  return self;
}

- (instancetype)initWithSerializedFileURL:(NSURL*)fileURL
                                    error:(NSError**)error {
  if ((self = [super init])) {
    const auto data = ReadFileBytes(base::apple::NSURLToFilePath(fileURL));
    if (!data.has_value()) {
      if (error) {
        *error = [[self class]
            adblockErrorForKind:adblock::ResultKind::AdblockError
                        message:"Failed to read serialized data file"];
      }
      return nil;
    }

    if (!adblock_engine->deserialize(*data)) {
      if (error) {
        *error =
            [[self class] adblockErrorForKind:adblock::ResultKind::AdblockError
                                      message:"Failed to deserialize data"];
      }
      return nil;
    }
  }
  return self;
}

/// Replaces the engine with the one in `result`, or returns `NO` and populates
/// `error` if the engine could not be created.
- (BOOL)setEngineFromResult:(adblock::BoxEngineResult)result
                      error:(NSError**)error {
  if (result.result_kind != adblock::ResultKind::Success) {
    if (error) {
      *error = [[self class] adblockErrorForKind:result.result_kind
                                         message:result.error_message];
    }
    return NO;
  }
  adblock_engine = std::move(result.value);
  return YES;
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
      isThirdParty, "", previouslyMatchedRule, forceCheckExceptions);
  auto result = [[AdblockEngineMatchResult alloc] init];
  result.didMatchRule = engine_result.matched;
  result.didMatchException = engine_result.has_exception;
  result.didMatchImportant = engine_result.important;
  if (engine_result.redirect.has_value) {
    result.redirect = base::SysUTF8ToNSString(
        static_cast<std::string>(engine_result.redirect.value));
  }
  if (engine_result.rewritten_url.has_value) {
    ;
    result.rewrittenURL = base::SysUTF8ToNSString(
        static_cast<std::string>(engine_result.rewritten_url.value));
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
          base::SysNSStringToUTF8(resourceType), isThirdParty, "")));
}

- (BOOL)serializeToFileURL:(NSURL*)fileURL error:(NSError**)error {
  auto result = adblock_engine->serialize();

  if (result.empty()) {
    if (error) {
      *error =
          [[self class] adblockErrorForKind:adblock::ResultKind::AdblockError
                                    message:"Failed to serialize data"];
    }
    return NO;
  }

  // SAFETY: `rust::Vec` is a contiguous buffer, so its data pointer and size
  // always describe a valid range, and `result` outlives the write below.
  // Constructing the span from the container instead would instantiate
  // `rust::Slice::end()`, whose own pointer arithmetic is not spanified.
  const auto span = UNSAFE_BUFFERS(base::span(result.data(), result.size()));

  const base::FilePath path = base::apple::NSURLToFilePath(fileURL);
  if (!base::WriteFile(path, span)) {
    // Don't leave a partially written file behind
    base::DeleteFile(path);
    if (error) {
      *error = [[self class]
          adblockErrorForKind:adblock::ResultKind::AdblockError
                      message:"Failed to write serialized data to file"];
    }
    return NO;
  }

  return YES;
}

- (bool)useResources:(NSString*)resources {
  // TODO(https://github.com/brave/brave-browser/issues/51103):
  // Reuse the once created storage for the both engines.
  auto storage =
      adblock::new_resource_storage(base::SysNSStringToUTF8(resources));
  adblock_engine->use_resource_storage(*storage);
  return true;
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
