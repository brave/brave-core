/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/ad_block_engine.h"

#include <algorithm>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/json/json_reader.h"
#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/adblock_rust_ffi/src/wrapper.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "brave/components/brave_shields/common/block_decision.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/origin.h"

using namespace net::registry_controlled_domains;  // NOLINT

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

AdBlockEngine::AdBlockEngine() : ad_block_client_(new adblock::Engine()) {}

AdBlockEngine::~AdBlockEngine() {}

void AdBlockEngine::ShouldStartRequest(
    const GURL& url,
    blink::mojom::ResourceType resource_type,
    const std::string& tab_host,
    bool aggressive_blocking,
    bool* did_match_rule,
    bool* did_match_exception,
    bool* did_match_important,
    std::string* mock_data_url,
    std::unique_ptr<BlockDecision>* block_decision) {
  // Determine third-party here so the library doesn't need to figure it out.
  // CreateFromNormalizedTuple is needed because SameDomainOrHost needs
  // a URL or origin and not a string to a host name.
  bool is_third_party = !SameDomainOrHost(
      url,
      url::Origin::CreateFromNormalizedTuple("https", tab_host.c_str(), 80),
      INCLUDE_PRIVATE_REGISTRIES);
  std::string filter;
  ad_block_client_->matches(url.spec(), url.host(), tab_host, is_third_party,
                            ResourceTypeToString(resource_type), did_match_rule,
                            did_match_exception, did_match_important, &filter,
                            mock_data_url);
  if (block_decision) {
    *block_decision = std::make_unique<AdBlockDecision>(filter);
  }

  // LOG(ERROR) << "AdBlockEngine::ShouldStartRequest(), host: "
  //  << tab_host
  //  << ", resource type: " << resource_type
  //  << ", url.spec(): " << url.spec();
}

absl::optional<std::string> AdBlockEngine::GetCspDirectives(
    const GURL& url,
    blink::mojom::ResourceType resource_type,
    const std::string& tab_host) {
  // Determine third-party here so the library doesn't need to figure it out.
  // CreateFromNormalizedTuple is needed because SameDomainOrHost needs
  // a URL or origin and not a string to a host name.
  bool is_third_party = !SameDomainOrHost(
      url,
      url::Origin::CreateFromNormalizedTuple("https", tab_host.c_str(), 80),
      INCLUDE_PRIVATE_REGISTRIES);
  const std::string result = ad_block_client_->getCspDirectives(
      url.spec(), url.host(), tab_host, is_third_party,
      ResourceTypeToString(resource_type));

  if (result.empty()) {
    return absl::nullopt;
  } else {
    return absl::optional<std::string>(result);
  }
}

void AdBlockEngine::EnableTag(const std::string& tag, bool enabled) {
  if (enabled) {
    if (tags_.find(tag) == tags_.end()) {
      ad_block_client_->addTag(tag);
      tags_.insert(tag);
    }
  } else {
    ad_block_client_->removeTag(tag);
    std::set<std::string>::iterator it =
        std::find(tags_.begin(), tags_.end(), tag);
    if (it != tags_.end()) {
      tags_.erase(it);
    }
  }
}

void AdBlockEngine::AddResources(const std::string& resources) {
  ad_block_client_->addResources(resources);
}

bool AdBlockEngine::TagExists(const std::string& tag) {
  return std::find(tags_.begin(), tags_.end(), tag) != tags_.end();
}

absl::optional<base::Value> AdBlockEngine::UrlCosmeticResources(
    const std::string& url) {
  return base::JSONReader::Read(ad_block_client_->urlCosmeticResources(url));
}

base::Value AdBlockEngine::HiddenClassIdSelectors(
    const std::vector<std::string>& classes,
    const std::vector<std::string>& ids,
    const std::vector<std::string>& exceptions) {
  absl::optional<base::Value> result = base::JSONReader::Read(
      ad_block_client_->hiddenClassIdSelectors(classes, ids, exceptions));

  if (!result) {
    return base::ListValue();
  } else {
    return std::move(*result);
  }
}

absl::optional<adblock::FilterListMetadata> AdBlockEngine::Load(
    bool deserialize,
    const DATFileDataBuffer& dat_buf,
    const std::string& resources_json) {
  if (deserialize) {
    OnDATLoaded(dat_buf, resources_json);
    return absl::nullopt;
  } else {
    return absl::make_optional(OnListSourceLoaded(dat_buf, resources_json));
  }
}

void AdBlockEngine::UpdateAdBlockClient(
    std::unique_ptr<adblock::Engine> ad_block_client,
    const std::string& resources_json) {
  ad_block_client_ = std::move(ad_block_client);
  AddResources(resources_json);
  AddKnownTagsToAdBlockInstance();
  if (test_observer_) {
    test_observer_->OnEngineUpdated();
  }
}

void AdBlockEngine::AddKnownTagsToAdBlockInstance() {
  std::for_each(tags_.begin(), tags_.end(),
                [&](const std::string tag) { ad_block_client_->addTag(tag); });
}

adblock::FilterListMetadata AdBlockEngine::OnListSourceLoaded(
    const DATFileDataBuffer& filters,
    const std::string& resources_json) {
  auto metadata_and_engine = adblock::engineFromBufferWithMetadata(
      reinterpret_cast<const char*>(filters.data()), filters.size());
  UpdateAdBlockClient(std::move(metadata_and_engine.second), resources_json);
  return std::move(metadata_and_engine.first);
}

void AdBlockEngine::OnDATLoaded(const DATFileDataBuffer& dat_buf,
                                const std::string& resources_json) {
  // An empty buffer will not load successfully.
  if (dat_buf.empty()) {
    return;
  }

  auto client = std::make_unique<adblock::Engine>();
  client->deserialize(reinterpret_cast<const char*>(&dat_buf.front()),
                      dat_buf.size());

  UpdateAdBlockClient(std::move(client), resources_json);
}

void AdBlockEngine::AddObserverForTest(AdBlockEngine::TestObserver* observer) {
  test_observer_ = observer;
}

void AdBlockEngine::RemoveObserverForTest() {
  test_observer_ = nullptr;
}

}  // namespace brave_shields
