// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/content/browser/ad_block_engine.h"

#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/metrics/histogram_functions.h"
#include "base/strings/string_number_conversions.h"
#include "base/timer/elapsed_timer.h"
#include "base/trace_event/trace_event.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/origin.h"

using net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES;

namespace {

std::string ResourceTypeToString(blink::mojom::ResourceType resource_type) {
  std::string filter_option = "";
  switch (resource_type) {
    // top level page
    case blink::mojom::ResourceType::kMainFrame:
      filter_option = "main_frame";
      break;
    // frame or iframe
    case blink::mojom::ResourceType::kSubFrame:
      filter_option = "sub_frame";
      break;
    // a CSS stylesheet
    case blink::mojom::ResourceType::kStylesheet:
      filter_option = "stylesheet";
      break;
    // an external script
    case blink::mojom::ResourceType::kScript:
      filter_option = "script";
      break;
    // an image (jpg/gif/png/etc)
    case blink::mojom::ResourceType::kFavicon:
    case blink::mojom::ResourceType::kImage:
      filter_option = "image";
      break;
    // a font
    case blink::mojom::ResourceType::kFontResource:
      filter_option = "font";
      break;
    // an "other" subresource.
    case blink::mojom::ResourceType::kSubResource:
      filter_option = "other";
      break;
    // an object (or embed) tag for a plugin.
    case blink::mojom::ResourceType::kObject:
      filter_option = "object";
      break;
    // a media resource.
    case blink::mojom::ResourceType::kMedia:
      filter_option = "media";
      break;
    // a XMLHttpRequest
    case blink::mojom::ResourceType::kXhr:
      filter_option = "xhr";
      break;
    // a ping request for <a ping>/sendBeacon.
    case blink::mojom::ResourceType::kPing:
      filter_option = "ping";
      break;
    // the main resource of a dedicated worker.
    case blink::mojom::ResourceType::kWorker:
    // the main resource of a shared worker.
    case blink::mojom::ResourceType::kSharedWorker:
    // an explicitly requested prefetch
    case blink::mojom::ResourceType::kPrefetch:
    // the main resource of a service worker.
    case blink::mojom::ResourceType::kServiceWorker:
    // a report of Content Security Policy violations.
    case blink::mojom::ResourceType::kCspReport:
    // a resource that a plugin requested.
    case blink::mojom::ResourceType::kPluginResource:
    default:
      break;
  }
  return filter_option;
}

}  // namespace

namespace brave_shields {

AdBlockEngine::AdBlockEngine(bool is_default_engine)
    : ad_block_client_(adblock::new_engine()),
      is_default_engine_(is_default_engine) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

AdBlockEngine::~AdBlockEngine() = default;

adblock::BlockerResult AdBlockEngine::ShouldStartRequest(
    const GURL& url,
    blink::mojom::ResourceType resource_type,
    const std::string& tab_host,
    bool previously_matched_rule,
    bool previously_matched_exception,
    bool previously_matched_important) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Determine third-party here so the library doesn't need to figure it out.
  // CreateFromNormalizedTuple is needed because SameDomainOrHost needs
  // a URL or origin and not a string to a host name.
  bool is_third_party = !SameDomainOrHost(
      url,
      url::Origin::CreateFromNormalizedTuple("https", tab_host.c_str(), 80),
      INCLUDE_PRIVATE_REGISTRIES);
  return ad_block_client_->matches(
      url.spec(), url.host(), tab_host, ResourceTypeToString(resource_type),
      is_third_party,
      // Checking normal rules is skipped if a normal rule or exception rule was
      // found previously
      previously_matched_rule || previously_matched_exception,
      // Always check exceptions unless one was found previously
      !previously_matched_exception);
}

std::optional<std::string> AdBlockEngine::GetCspDirectives(
    const GURL& url,
    blink::mojom::ResourceType resource_type,
    const std::string& tab_host) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Determine third-party here so the library doesn't need to figure it out.
  // CreateFromNormalizedTuple is needed because SameDomainOrHost needs
  // a URL or origin and not a string to a host name.
  bool is_third_party = !SameDomainOrHost(
      url,
      url::Origin::CreateFromNormalizedTuple("https", tab_host.c_str(), 80),
      INCLUDE_PRIVATE_REGISTRIES);
  auto result = ad_block_client_->get_csp_directives(
      url.spec(), url.host(), tab_host, ResourceTypeToString(resource_type),
      is_third_party);

  if (result.empty()) {
    return std::nullopt;
  } else {
    return std::optional<std::string>(std::string(result));
  }
}

void AdBlockEngine::EnableTag(const std::string& tag, bool enabled) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (enabled) {
    if (tags_.find(tag) == tags_.end()) {
      ad_block_client_->enable_tag(tag);
      tags_.insert(tag);
    }
  } else {
    ad_block_client_->disable_tag(tag);
    tags_.erase(tag);
  }
}

void AdBlockEngine::UseResources(const std::string& resources) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  bool result = ad_block_client_->use_resources(resources);
  if (!result) {
    LOG(ERROR) << "AdBlockEngine::UseResources failed";
  }
}

bool AdBlockEngine::TagExists(const std::string& tag) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return base::Contains(tags_, tag);
}

base::Value::Dict AdBlockEngine::GetDebugInfo() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  const auto debug_info_struct = ad_block_client_->get_regex_debug_info();
  base::Value::List regex_list;
  for (const auto& regex_entry : debug_info_struct.regex_data) {
    base::Value::Dict regex_info;
    regex_info.Set("id", base::NumberToString(regex_entry.id));
    regex_info.Set("regex", std::string(regex_entry.regex.value));
    regex_info.Set("unused_sec", static_cast<int>(regex_entry.unused_secs));
    regex_info.Set("usage_count", static_cast<int>(regex_entry.usage_count));
    regex_list.Append(std::move(regex_info));
  }

  base::Value::Dict result;
  result.Set("compiled_regex_count",
             static_cast<int>(debug_info_struct.compiled_regex_count));
  result.Set("regex_data", std::move(regex_list));
  return result;
}

void AdBlockEngine::DiscardRegex(uint64_t regex_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  ad_block_client_->discard_regex(regex_id);
}

void AdBlockEngine::SetupDiscardPolicy(
    const adblock::RegexManagerDiscardPolicy& policy) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  regex_discard_policy_ = policy;
  ad_block_client_->set_regex_discard_policy(policy);
}

base::Value::Dict AdBlockEngine::UrlCosmeticResources(const std::string& url) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto result = ad_block_client_->url_cosmetic_resources(url);

  std::optional<base::Value> parsed_result =
      base::JSONReader::Read(result.c_str());

  if (!parsed_result) {
    return base::Value::Dict();
  } else {
    DCHECK(parsed_result->is_dict());
    return std::move(parsed_result->GetDict());
  }
}

base::Value::List AdBlockEngine::HiddenClassIdSelectors(
    const std::vector<std::string>& classes,
    const std::vector<std::string>& ids,
    const std::vector<std::string>& exceptions) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto result =
      ad_block_client_->hidden_class_id_selectors(classes, ids, exceptions);
  if (result.result_kind != adblock::ResultKind::Success) {
    LOG(ERROR) << "AdBlockEngine::HiddenClassIdSelectors failed: "
               << result.error_message.c_str();
    return base::Value::List();
  }

  base::Value::List list_result;
  for (const auto& selector : result.value) {
    list_result.Append(std::string(selector));
  }

  return list_result;
}

void AdBlockEngine::Load(bool deserialize,
                         const DATFileDataBuffer& dat_buf,
                         const std::string& resources_json) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (deserialize) {
    OnDATLoaded(dat_buf, resources_json);
  } else {
    OnListSourceLoaded(dat_buf, resources_json);
  }
}

void AdBlockEngine::Load(rust::Box<adblock::FilterSet> filter_set,
                         const std::string& resources_json) {
  OnFilterSetLoaded(std::move(filter_set), resources_json);
}

void AdBlockEngine::UpdateAdBlockClient(
    rust::Box<adblock::Engine> ad_block_client,
    const std::string& resources_json) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  ad_block_client_ = std::move(ad_block_client);
  if (regex_discard_policy_) {
    ad_block_client_->set_regex_discard_policy(*regex_discard_policy_);
  }
  UseResources(resources_json);
  AddKnownTagsToAdBlockInstance();
  if (test_observer_) {
    test_observer_->OnEngineUpdated();
  }
}

void AdBlockEngine::AddKnownTagsToAdBlockInstance() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::for_each(tags_.begin(), tags_.end(), [&](const std::string& tag) {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    ad_block_client_->enable_tag(tag);
  });
}

void AdBlockEngine::OnFilterSetLoaded(rust::Box<adblock::FilterSet> filter_set,
                                      const std::string& resources_json) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  base::ElapsedTimer timer;
  TRACE_EVENT_BEGIN1("brave.adblock", "MakeEngineWithRules",
                     "is_default_engine", is_default_engine_);

  auto result = adblock::engine_from_filter_set(std::move(filter_set));

  TRACE_EVENT_END0("brave.adblock", "MakeEngineWithRules");
  if (is_default_engine_) {
    base::UmaHistogramTimes("Brave.Adblock.MakeEngineWithRules.Default",
                            timer.Elapsed());
  } else {
    base::UmaHistogramTimes("Brave.Adblock.MakeEngineWithRules.Additional",
                            timer.Elapsed());
  }

  if (result.result_kind != adblock::ResultKind::Success) {
    VLOG(0) << "AdBlockEngine::OnFilterSetLoaded failed: "
            << result.error_message.c_str();
    return;
  }
  UpdateAdBlockClient(std::move(result.value), resources_json);
}

void AdBlockEngine::OnListSourceLoaded(const DATFileDataBuffer& filters,
                                       const std::string& resources_json) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  base::ElapsedTimer timer;
  TRACE_EVENT_BEGIN2("brave.adblock", "MakeEngineWithRules", "size",
                     filters.size(), "is_default_engine", is_default_engine_);

  auto result = adblock::engine_with_rules(filters);

  TRACE_EVENT_END0("brave.adblock", "MakeEngineWithRules");
  if (is_default_engine_) {
    base::UmaHistogramTimes("Brave.Adblock.MakeEngineWithRules.Default",
                            timer.Elapsed());
  } else {
    base::UmaHistogramTimes("Brave.Adblock.MakeEngineWithRules.Additional",
                            timer.Elapsed());
  }

  if (result.result_kind != adblock::ResultKind::Success) {
    LOG(ERROR) << "AdBlockEngine::OnListSourceLoaded failed: "
               << result.error_message.c_str();
    return;
  }
  UpdateAdBlockClient(std::move(result.value), resources_json);
}

void AdBlockEngine::OnDATLoaded(const DATFileDataBuffer& dat_buf,
                                const std::string& resources_json) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // An empty buffer will not load successfully.
  if (dat_buf.empty()) {
    return;
  }

  base::ElapsedTimer timer;
  TRACE_EVENT_BEGIN2("brave.adblock", "EngineDeserialize", "size",
                     dat_buf.size(), "is_default_engine", is_default_engine_);

  auto client = adblock::new_engine();
  const auto result = client->deserialize(dat_buf);

  TRACE_EVENT_END0("brave.adblock", "EngineDeserialize");
  if (is_default_engine_) {
    base::UmaHistogramTimes("Brave.Adblock.EngineDeserialize.Default",
                            timer.Elapsed());
  } else {
    base::UmaHistogramTimes("Brave.Adblock.EngineDeserialize.Additional",
                            timer.Elapsed());
  }

  if (!result) {
    LOG(ERROR) << "AdBlockEngine::OnDATLoaded deserialize failed";
    return;
  }

  UpdateAdBlockClient(std::move(client), resources_json);
}

void AdBlockEngine::AddObserverForTest(AdBlockEngine::TestObserver* observer) {
  test_observer_ = observer;
}

void AdBlockEngine::RemoveObserverForTest() {
  test_observer_ = nullptr;
}

base::WeakPtr<AdBlockEngine> AdBlockEngine::AsWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

}  // namespace brave_shields
