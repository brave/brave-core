// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/ad_block_engine.h"

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/containers/to_vector.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/metrics/histogram_functions.h"
#include "base/sequence_checker.h"
#include "base/strings/string_number_conversions.h"
#include "base/timer/elapsed_timer.h"
#include "base/trace_event/trace_event.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/origin.h"

using net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES;

namespace {

std::string_view RequestTypeToString(
    brave_shields::AdBlockRequestType request_type) {
  using brave_shields::AdBlockRequestType;
  switch (request_type) {
    case AdBlockRequestType::kMainFrame:
      return "main_frame";
    case AdBlockRequestType::kSubFrame:
      return "sub_frame";
    case AdBlockRequestType::kStylesheet:
      return "stylesheet";
    case AdBlockRequestType::kScript:
      return "script";
    case AdBlockRequestType::kImage:
      return "image";
    case AdBlockRequestType::kFont:
      return "font";
    case AdBlockRequestType::kObject:
      return "object";
    case AdBlockRequestType::kMedia:
      return "media";
    case AdBlockRequestType::kXhr:
      return "xhr";
    case AdBlockRequestType::kPing:
      return "ping";
    case AdBlockRequestType::kOther:
      return "other";
    case AdBlockRequestType::kUnknown:
      return "";
  }
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
    AdBlockRequestType request_type,
    const url::Origin& request_initiator,
    const std::string& method,
    bool previously_matched_rule,
    bool previously_matched_exception,
    bool previously_matched_important) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Determine third-party here so the library doesn't need to figure it out.
  bool is_third_party =
      !SameDomainOrHost(url, request_initiator, INCLUDE_PRIVATE_REGISTRIES);
  const auto initiator_hostname = request_initiator.host();
  return ad_block_client_->matches(
      url.spec(), std::string(url.host()), initiator_hostname,
      std::string(RequestTypeToString(request_type)), is_third_party, method,
      // Checking normal rules is skipped if a normal rule or exception rule was
      // found previously
      previously_matched_rule || previously_matched_exception,
      // Always check exceptions unless one was found previously
      !previously_matched_exception);
}

std::optional<std::string> AdBlockEngine::GetCspDirectives(
    const GURL& url,
    AdBlockRequestType request_type,
    const url::Origin& first_party_origin,
    const std::string& method) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Determine third-party here so the library doesn't need to figure it out.
  bool is_third_party =
      !SameDomainOrHost(url, first_party_origin, INCLUDE_PRIVATE_REGISTRIES);

  auto result = ad_block_client_->get_csp_directives(
      url.spec(), std::string(url.host()), first_party_origin.host(),
      std::string(RequestTypeToString(request_type)), is_third_party, method);

  if (result.empty()) {
    return std::nullopt;
  } else {
    return std::optional<std::string>(std::string(result));
  }
}

void AdBlockEngine::UseResources(
    const adblock::BraveCoreResourceStorage& storage) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  ad_block_client_->use_resource_storage(storage);
}

base::DictValue AdBlockEngine::GetDebugInfo() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  const auto debug_info_struct = ad_block_client_->get_debug_info();
  base::ListValue regex_list;
  for (const auto& regex_entry : debug_info_struct.regex_data) {
    base::DictValue regex_info;
    regex_info.Set("id", base::NumberToString(regex_entry.id));
    regex_info.Set("regex", std::string(regex_entry.regex.value));
    regex_info.Set("unused_sec", static_cast<int>(regex_entry.unused_secs));
    regex_info.Set("usage_count", static_cast<int>(regex_entry.usage_count));
    regex_list.Append(std::move(regex_info));
  }

  base::DictValue result;
  result.Set("compiled_regex_count",
             static_cast<int>(debug_info_struct.compiled_regex_count));
  result.Set("flatbuffer_size",
             static_cast<int>(debug_info_struct.flatbuffer_size));
  result.Set("regex_data", std::move(regex_list));

  base::ListValue source_info;
  for (const auto& item : debug_info_struct.source_info) {
    base::DictValue source_info_dict;
    source_info_dict.Set("title", std::string(item.title.value));
    source_info_dict.Set("homepage", std::string(item.homepage.value));
    source_info_dict.Set("network_filter_count",
                         static_cast<int>(item.network_filter_count));
    source_info_dict.Set("cosmetic_filter_count",
                         static_cast<int>(item.cosmetic_filter_count));
    source_info_dict.Set("parse_error_count",
                         static_cast<int>(item.parse_error));
    std::string invalid_lines_concatenated;
    for (const auto& invalid_line : item.invalid_lines) {
      invalid_lines_concatenated.append(
          std::string_view(invalid_line.data(), invalid_line.size()));
      invalid_lines_concatenated.append("\n");
    }
    source_info_dict.Set("invalid_lines",
                         std::move(invalid_lines_concatenated));

    source_info.Append(std::move(source_info_dict));
  }
  result.Set("source_info", std::move(source_info));
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

base::DictValue AdBlockEngine::UrlCosmeticResources(const std::string& url) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto result = ad_block_client_->url_cosmetic_resources(url);

  std::optional<base::DictValue> parsed_result = base::JSONReader::ReadDict(
      std::string_view(result.data(), result.size()), base::JSON_PARSE_RFC);
  if (!parsed_result) {
    return base::DictValue();
  }
  return std::move(parsed_result).value();
}

base::ListValue AdBlockEngine::HiddenClassIdSelectors(
    const std::vector<std::string>& classes,
    const std::vector<std::string>& ids,
    const std::vector<std::string>& exceptions) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto result =
      ad_block_client_->hidden_class_id_selectors(classes, ids, exceptions);
  if (result.result_kind != adblock::ResultKind::Success) {
    LOG(ERROR) << "AdBlockEngine::HiddenClassIdSelectors failed: "
               << result.error_message.c_str();
    return base::ListValue();
  }

  base::ListValue list_result;
  for (const auto& selector : result.value) {
    list_result.Append(std::string(selector));
  }

  return list_result;
}

bool AdBlockEngine::Load(bool deserialize,
                         const DATFileDataBuffer& dat_buf,
                         const adblock::BraveCoreResourceStorage& storage) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (deserialize) {
    return OnDATLoaded(dat_buf, storage);
  } else {
    return OnListSourceLoaded(dat_buf, storage);
  }
}

bool AdBlockEngine::Load(rust::Box<adblock::FilterSet> filter_set,
                         const adblock::BraveCoreResourceStorage& storage) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return OnFilterSetLoaded(std::move(filter_set), storage);
}

void AdBlockEngine::UpdateAdBlockClient(
    rust::Box<adblock::Engine> ad_block_client,
    const adblock::BraveCoreResourceStorage& storage) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  TRACE_EVENT("brave.adblock", "UpdateAdBlockClient");
  ad_block_client_ = std::move(ad_block_client);
  if (regex_discard_policy_) {
    ad_block_client_->set_regex_discard_policy(*regex_discard_policy_);
  }
  UseResources(storage);
}

bool AdBlockEngine::OnFilterSetLoaded(
    rust::Box<adblock::FilterSet> filter_set,
    const adblock::BraveCoreResourceStorage& storage) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  base::ElapsedTimer timer;
  TRACE_EVENT_BEGIN("brave.adblock", "MakeEngineWithRules", "is_default_engine",
                    is_default_engine_);

  auto result = adblock::engine_from_filter_set(std::move(filter_set));

  TRACE_EVENT_END("brave.adblock");
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
    return false;
  }
  UpdateAdBlockClient(std::move(result.value), storage);
  return true;
}

bool AdBlockEngine::OnListSourceLoaded(
    const DATFileDataBuffer& filters,
    const adblock::BraveCoreResourceStorage& storage) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  base::ElapsedTimer timer;
  TRACE_EVENT_BEGIN("brave.adblock", "MakeEngineWithRules", "size",
                    filters.size(), "is_default_engine", is_default_engine_);

  auto result = adblock::engine_with_rules(filters);

  TRACE_EVENT_END("brave.adblock");
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
    return false;
  }
  UpdateAdBlockClient(std::move(result.value), storage);
  return true;
}

bool AdBlockEngine::OnDATLoaded(
    const DATFileDataBuffer& dat_buf,
    const adblock::BraveCoreResourceStorage& storage) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // An empty buffer will not load successfully.
  if (dat_buf.empty()) {
    return false;
  }

  base::ElapsedTimer timer;
  TRACE_EVENT_BEGIN("brave.adblock", "EngineDeserialize", "size",
                    dat_buf.size(), "is_default_engine", is_default_engine_);

  auto client = adblock::new_engine();
  const auto result = client->deserialize(dat_buf);

  TRACE_EVENT_END("brave.adblock");
  if (is_default_engine_) {
    base::UmaHistogramTimes("Brave.Adblock.EngineDeserialize.Default",
                            timer.Elapsed());
  } else {
    base::UmaHistogramTimes("Brave.Adblock.EngineDeserialize.Additional",
                            timer.Elapsed());
  }

  if (!result) {
    LOG(ERROR) << "AdBlockEngine::OnDATLoaded deserialize failed";
    return false;
  }

  UpdateAdBlockClient(std::move(client), storage);
  return true;
}

DATFileDataBuffer AdBlockEngine::Serialize() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return base::ToVector(ad_block_client_->serialize());
}

}  // namespace brave_shields
