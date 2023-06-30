/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_ADBLOCK_RUST_FFI_SRC_WRAPPER_H_
#define BRAVE_COMPONENTS_ADBLOCK_RUST_FFI_SRC_WRAPPER_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

extern "C" {
#include "brave/components/adblock_rust_ffi/src/lib.h"
}

#if defined(ADBLOCK_SHARED_LIBRARY)
#if defined(WIN32)
#if defined(ADBLOCK_IMPLEMENTATION)
#define ADBLOCK_EXPORT __declspec(dllexport)
#else
#define ADBLOCK_EXPORT __declspec(dllimport)
#endif  // defined(ADBLOCK_IMPLEMENTATION)
#else   // defined(WIN32)
#if defined(ADBLOCK_IMPLEMENTATION)
#define ADBLOCK_EXPORT __attribute__((visibility("default")))
#else
#define ADBLOCK_EXPORT
#endif  // defined(ADBLOCK_IMPLEMENTATION)
#endif
#else  // defined(ADBLOCK_SHARED_LIBRARY)
#define ADBLOCK_EXPORT
#endif

namespace adblock {

typedef C_DomainResolverCallback DomainResolverCallback;

bool ADBLOCK_EXPORT SetDomainResolver(DomainResolverCallback resolver);

#if BUILDFLAG(IS_IOS)
const std::string ADBLOCK_EXPORT
ConvertRulesToContentBlockingRules(const std::string& rules, bool* truncated);
#endif

const ADBLOCK_EXPORT uint16_t kSubscriptionDefaultExpiresHours =
    SUBSCRIPTION_DEFAULT_EXPIRES_HOURS;

typedef ADBLOCK_EXPORT struct FilterListMetadata {
  FilterListMetadata();
  explicit FilterListMetadata(C_FilterListMetadata* metadata);
  explicit FilterListMetadata(const std::string& list);
  explicit FilterListMetadata(const char* data, size_t data_size);
  ~FilterListMetadata();

  absl::optional<std::string> homepage;
  absl::optional<std::string> title;
  // Normalized to a value in hours
  uint16_t expires = kSubscriptionDefaultExpiresHours;

  FilterListMetadata(FilterListMetadata&&);

  FilterListMetadata(const FilterListMetadata&) = delete;
} FilterListMetadata;

// C++ version of adblock-rust:RegexDebugEntry struct.
struct ADBLOCK_EXPORT RegexDebugEntry {
  uint64_t id;
  std::string regex;
  uint64_t unused_sec;
  size_t usage_count;
};

// C++ version of adblock-rust:RegexManagerDiscardPolicy struct.
struct ADBLOCK_EXPORT RegexManagerDiscardPolicy {
  uint64_t cleanup_interval_sec;
  uint64_t discard_unused_sec;
};

// C++ version of adblock-rust:EngineDebugInfo struct.
struct ADBLOCK_EXPORT AdblockDebugInfo {
  std::vector<RegexDebugEntry> regex_data;
  size_t compiled_regex_count;

  AdblockDebugInfo();
  AdblockDebugInfo(const AdblockDebugInfo&);
  ~AdblockDebugInfo();
};

class ADBLOCK_EXPORT Engine {
 public:
  Engine();
  explicit Engine(C_Engine* c_engine);
  explicit Engine(const std::string& rules);
  Engine(const char* data, size_t data_size);
  void matches(const std::string& url,
               const std::string& host,
               const std::string& tab_host,
               bool is_third_party,
               const std::string& resource_type,
               bool* did_match_rule,
               bool* did_match_exception,
               bool* did_match_important,
               std::string* redirect,
               std::string* rewritten_url);
  std::string getCspDirectives(const std::string& url,
                               const std::string& host,
                               const std::string& tab_host,
                               bool is_third_party,
                               const std::string& resource_type);
  bool deserialize(const char* data, size_t data_size);
  void addTag(const std::string& tag);
  void addResource(const std::string& key,
                   const std::string& content_type,
                   const std::string& data);
  void useResources(const std::string& resources);
  void removeTag(const std::string& tag);
  bool tagExists(const std::string& tag);
  const std::string urlCosmeticResources(const std::string& url);
  const std::string hiddenClassIdSelectors(
      const std::vector<std::string>& classes,
      const std::vector<std::string>& ids,
      const std::vector<std::string>& exceptions);
  AdblockDebugInfo getAdblockDebugInfo();
  void discardRegex(uint64_t regex_id);
  void setupDiscardPolicy(const RegexManagerDiscardPolicy& policy);

  ~Engine();

  Engine(Engine&&) = default;

 private:
  Engine(const Engine&) = delete;
  void operator=(const Engine&) = delete;
  raw_ptr<C_Engine> raw = nullptr;
};

std::pair<FilterListMetadata, std::unique_ptr<Engine>> engineWithMetadata(
    const std::string& rules);
std::pair<FilterListMetadata, std::unique_ptr<Engine>>
engineFromBufferWithMetadata(const char* data, size_t data_size);

}  // namespace adblock

#endif  // BRAVE_COMPONENTS_ADBLOCK_RUST_FFI_SRC_WRAPPER_H_
