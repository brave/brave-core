// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/content/browser/ad_block_engine_wrapper.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/debug/leak_annotations.h"
#include "base/feature_list.h"
#include "base/json/json_reader.h"
#include "base/sequence_checker.h"
#include "base/strings/strcat.h"
#include "base/trace_event/trace_event.h"
#include "base/values.h"
#include "brave/components/brave_shields/content/browser/ad_block_engine.h"
#include "brave/components/brave_shields/core/browser/ad_block_resource_provider.h"
#include "brave/components/brave_shields/core/browser/ad_block_service_helper.h"
#include "brave/components/brave_shields/core/common/adblock/rs/src/lib.rs.h"
#include "brave/components/brave_shields/core/common/brave_shield_constants.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom-shared.h"
#include "url/origin.h"

namespace brave_shields {

AdBlockEngineWrapper::AdBlockEngineWrapper(
    std::unique_ptr<AdBlockEngine> default_engine,
    std::unique_ptr<AdBlockEngine> additional_engine)
    : default_engine_(std::move(default_engine)),
      additional_filters_engine_(std::move(additional_engine)) {
  // `this` is stored using SequenceBound, so false-positive shutdown leaks
  // are expected
  ANNOTATE_LEAKING_OBJECT_PTR(this);
}

AdBlockEngineWrapper::~AdBlockEngineWrapper() = default;

// static
std::unique_ptr<AdBlockEngineWrapper> AdBlockEngineWrapper::Create() {
  return std::make_unique<AdBlockEngineWrapper>(
      std::make_unique<AdBlockEngine>(true /* is_default */),
      std::make_unique<AdBlockEngine>(false /* is_default */));
}

adblock::BlockerResult AdBlockEngineWrapper::ShouldStartRequest(
    const GURL& url,
    blink::mojom::ResourceType resource_type,
    const std::string& tab_host,
    bool aggressive_blocking,
    bool previously_matched_rule,
    bool previously_matched_exception,
    bool previously_matched_important) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  TRACE_EVENT("brave.adblock", "ShouldStartRequest", "url", url);

  adblock::BlockerResult fp_result = default_engine_->ShouldStartRequest(
      url, resource_type, tab_host, previously_matched_rule,
      previously_matched_exception, previously_matched_important);

  // removeparam results from the default engine are always ignored
  fp_result.rewritten_url.has_value = false;

  if (aggressive_blocking ||
      base::FeatureList::IsEnabled(
          brave_shields::features::kBraveAdblockDefault1pBlocking) ||
      !SameDomainOrHost(
          url, url::Origin::CreateFromNormalizedTuple("https", tab_host, 80),
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES)) {
    if (fp_result.important) {
      return fp_result;
    }
  } else {
    // if there's an exception from the default engine, it still needs to be
    // considered by the additional engine
    fp_result = {.has_exception = fp_result.has_exception};
  }

  GURL request_url = fp_result.rewritten_url.has_value
                         ? GURL(std::string(fp_result.rewritten_url.value))
                         : url;
  auto result = additional_filters_engine_->ShouldStartRequest(
      request_url, resource_type, tab_host,
      previously_matched_rule | fp_result.matched,
      previously_matched_exception | fp_result.has_exception,
      previously_matched_important | fp_result.important);

  result.matched |= fp_result.matched;
  result.has_exception |= fp_result.has_exception;
  result.important |= fp_result.important;
  if (!result.redirect.has_value && fp_result.redirect.has_value) {
    result.redirect = fp_result.redirect;
  }
  if (!result.rewritten_url.has_value && fp_result.rewritten_url.has_value) {
    result.rewritten_url = fp_result.rewritten_url;
  }
  return result;
}

bool AdBlockEngineWrapper::Load(
    bool is_default_engine,
    std::unique_ptr<rust::Box<adblock::FilterSet>> filter_set,
    AdblockResourceStorageBox storage) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto* engine = is_default_engine ? default_engine_.get()
                                   : additional_filters_engine_.get();
  if (filter_set) {
    return engine->Load(std::move(*filter_set), *storage);
  } else {
    engine->UseResources(*storage);
    return true;
  }
}

bool AdBlockEngineWrapper::LoadDAT(bool is_default_engine,
                                   DATFileDataBuffer dat,
                                   AdblockResourceStorageBox storage) {
  CHECK(base::FeatureList::IsEnabled(features::kAdblockDATCache));
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto* engine = is_default_engine ? default_engine_.get()
                                   : additional_filters_engine_.get();
  if (!dat.empty()) {
    return engine->Load(true, std::move(dat), *storage);
  } else {
    engine->UseResources(*storage);
    return true;
  }
}

DATFileDataBuffer AdBlockEngineWrapper::Serialize(bool is_default_engine) {
  CHECK(base::FeatureList::IsEnabled(features::kAdblockDATCache));
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto* engine = is_default_engine ? default_engine_.get()
                                   : additional_filters_engine_.get();
  return engine->Serialize();
}

std::optional<std::string> AdBlockEngineWrapper::GetCspDirectives(
    const GURL& url,
    blink::mojom::ResourceType resource_type,
    const std::string& tab_host) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  TRACE_EVENT("brave.adblock", "GetCspDirectives", "url", url);
  auto csp_directives =
      default_engine_->GetCspDirectives(url, resource_type, tab_host);

  const auto additional_csp = additional_filters_engine_->GetCspDirectives(
      url, resource_type, tab_host);
  MergeCspDirectiveInto(additional_csp, &csp_directives);

  return csp_directives;
}

void AdBlockEngineWrapper::EnableTag(const std::string& tag, bool enabled) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Tags only need to be modified for the default engine.
  default_engine_->EnableTag(tag, enabled);
}

void AdBlockEngineWrapper::UseResources(
    const adblock::BraveCoreResourceStorage& storage) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  default_engine_->UseResources(storage);
  additional_filters_engine_->UseResources(storage);
}

bool AdBlockEngineWrapper::TagExists(const std::string& tag) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return default_engine_->TagExists(tag);
}

std::pair<base::DictValue, base::DictValue>
AdBlockEngineWrapper::GetDebugInfo() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return {default_engine_->GetDebugInfo(),
          additional_filters_engine_->GetDebugInfo()};
}

void AdBlockEngineWrapper::DiscardRegex(uint64_t regex_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Dispatch to both engines since regex IDs are unique across engines.
  default_engine_->DiscardRegex(regex_id);
  additional_filters_engine_->DiscardRegex(regex_id);
}

void AdBlockEngineWrapper::SetupDiscardPolicy(
    const adblock::RegexManagerDiscardPolicy& policy) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  default_engine_->SetupDiscardPolicy(policy);
  additional_filters_engine_->SetupDiscardPolicy(policy);
}

base::DictValue AdBlockEngineWrapper::UrlCosmeticResources(
    const std::string& url,
    bool aggressive_blocking) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  TRACE_EVENT("brave.adblock", "UrlCosmeticResources", "url", url);

  base::DictValue resources = default_engine_->UrlCosmeticResources(url);

  if (!aggressive_blocking) {
    // `:has` procedural selectors from the default engine should not be hidden
    // in standard blocking mode.
    base::ListValue* default_hide_selectors =
        resources.FindList("hide_selectors");
    if (default_hide_selectors) {
      base::ListValue::iterator it = default_hide_selectors->begin();
      while (it < default_hide_selectors->end()) {
        DCHECK(it->is_string());
        if (it->GetString().find(":has(") != std::string::npos) {
          it = default_hide_selectors->erase(it);
        } else {
          it++;
        }
      }
    }

    // In standard blocking mode, drop procedural filters but otherwise keep
    // action filters.
    StripProceduralFilters(resources);
  }

  base::DictValue additional_resources =
      additional_filters_engine_->UrlCosmeticResources(url);

  MergeResourcesInto(std::move(additional_resources), resources,
                     /*force_hide=*/true);

  return resources;
}

base::DictValue AdBlockEngineWrapper::HiddenClassIdSelectors(
    const std::vector<std::string>& classes,
    const std::vector<std::string>& ids,
    const std::vector<std::string>& exceptions) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  TRACE_EVENT("brave.adblock", "HiddenClassIdSelectors", "classes", classes,
              "ids", ids);

  base::ListValue hide_selectors =
      default_engine_->HiddenClassIdSelectors(classes, ids, exceptions);

  base::ListValue force_hide_selectors =
      additional_filters_engine_->HiddenClassIdSelectors(classes, ids,
                                                         exceptions);

  base::DictValue result;
  result.Set("hide_selectors", std::move(hide_selectors));
  result.Set("force_hide_selectors", std::move(force_hide_selectors));
  return result;
}

// Removes any procedural filters from the given UrlCosmeticResources Value.
//
// Procedural filters are filters with at least one selector operator of a type
// that isn't `css-selector`.
//
// These filters are represented as JSON provided by adblock-rust. The format
// is documented at:
// https://docs.rs/adblock/latest/adblock/cosmetic_filter_cache/struct.ProceduralOrActionFilter.html
// static
void AdBlockEngineWrapper::StripProceduralFilters(base::DictValue& resources) {
  TRACE_EVENT("brave.adblock", "StripProceduralFilters");
  base::ListValue* procedural_actions =
      resources.FindList(kCosmeticResourcesProceduralActions);
  if (procedural_actions) {
    base::ListValue::iterator it = procedural_actions->begin();
    while (it < procedural_actions->end()) {
      DCHECK(it->is_string());
      auto* pfilter_str = it->GetIfString();
      if (pfilter_str == nullptr) {
        continue;
      }
      auto val = base::JSONReader::ReadDict(*pfilter_str, base::JSON_PARSE_RFC);
      if (val) {
        auto* list = val->FindList("selector");
        if (list && list->size() != 1) {
          // Non-procedural filters are always a single operator in length.
          it = procedural_actions->erase(it);
          continue;
        }
        // The single operator must also be a `css-selector`.
        auto op_iterator = list->begin();
        auto* dict = op_iterator->GetIfDict();
        if (dict) {
          auto* str = dict->FindString("type");
          if (str && *str != "css-selector") {
            it = procedural_actions->erase(it);
            continue;
          }
        }
      }
      it++;
    }
  }
}

// Merges the contents of the first UrlCosmeticResources Value into the second
// one provided.
//
// If `force_hide` is true, the contents of `from`'s `hide_selectors` field
// will be moved into a possibly new field of `into` called
// `force_hide_selectors`.
void AdBlockEngineWrapper::MergeResourcesInto(base::DictValue from,
                                              base::DictValue& into,
                                              bool force_hide) {
  TRACE_EVENT("brave.adblock", "MergeResourcesInto");
  base::ListValue* resources_hide_selectors = nullptr;
  if (force_hide) {
    resources_hide_selectors = into.FindList("force_hide_selectors");
    if (!resources_hide_selectors) {
      resources_hide_selectors =
          into.Set("force_hide_selectors", base::ListValue())->GetIfList();
    }
  } else {
    resources_hide_selectors = into.FindList("hide_selectors");
  }
  base::ListValue* from_resources_hide_selectors =
      from.FindList("hide_selectors");
  if (resources_hide_selectors && from_resources_hide_selectors) {
    for (auto& selector : *from_resources_hide_selectors) {
      resources_hide_selectors->Append(std::move(selector));
    }
  }

  constexpr std::string_view kListKeys[] = {
      "exceptions", kCosmeticResourcesProceduralActions};
  for (const auto& key : kListKeys) {
    base::ListValue* resources = into.FindList(key);
    base::ListValue* from_resources = from.FindList(key);
    if (resources && from_resources) {
      for (auto& exception : *from_resources) {
        resources->Append(std::move(exception));
      }
    }
  }

  auto* resources_injected_script = into.FindString("injected_script");
  auto* from_resources_injected_script = from.FindString("injected_script");
  if (resources_injected_script && from_resources_injected_script) {
    *resources_injected_script = base::StrCat(
        {*resources_injected_script, "\n", *from_resources_injected_script});
  }

  auto from_resources_generichide = from.FindBool("generichide");
  if (from_resources_generichide && *from_resources_generichide) {
    into.Set("generichide", true);
  }
}

}  // namespace brave_shields
