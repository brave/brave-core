/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/ad_block_base_service.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/net/url_context.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_shields/browser/dat_file_util.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/vendor/ad-block/ad_block_client.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/prefs/pref_service.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/origin.h"

using namespace net::registry_controlled_domains;  // NOLINT

namespace {

FilterOption ResourceTypeToFilterOption(content::ResourceType resource_type) {
  FilterOption filter_option = FONoFilterOption;
  switch (resource_type) {
    // top level page
    case content::ResourceType::kMainFrame:
      filter_option = FODocument;
      break;
    // frame or iframe
    case content::ResourceType::kSubFrame:
      filter_option = FOSubdocument;
      break;
    // a CSS stylesheet
    case content::ResourceType::kStylesheet:
      filter_option = FOStylesheet;
      break;
    // an external script
    case content::ResourceType::kScript:
      filter_option = FOScript;
      break;
    // an image (jpg/gif/png/etc)
    case content::ResourceType::kFavicon:
    case content::ResourceType::kImage:
      filter_option = FOImage;
      break;
    // a font
    case content::ResourceType::kFontResource:
      filter_option = FOFont;
      break;
    // an "other" subresource.
    case content::ResourceType::kSubResource:
      filter_option = FOOther;
      break;
    // an object (or embed) tag for a plugin.
    case content::ResourceType::kObject:
      filter_option = FOObject;
      break;
    // a media resource.
    case content::ResourceType::kMedia:
      filter_option = FOMedia;
      break;
    // a XMLHttpRequest
    case content::ResourceType::kXhr:
      filter_option = FOXmlHttpRequest;
      break;
    // a ping request for <a ping>/sendBeacon.
    case content::ResourceType::kPing:
      filter_option = FOPing;
      break;
    // the main resource of a dedicated worker.
    case content::ResourceType::kWorker:
    // the main resource of a shared worker.
    case content::ResourceType::kSharedWorker:
    // an explicitly requested prefetch
    case content::ResourceType::kPrefetch:
    // the main resource of a service worker.
    case content::ResourceType::kServiceWorker:
    // a report of Content Security Policy violations.
    case content::ResourceType::kCspReport:
    // a resource that a plugin requested.
    case content::ResourceType::kPluginResource:
    // a service worker navigation preload request.
    case content::ResourceType::kNavigationPreload:
    // an invalid type (see brave/browser/net/url_context.h)
    case brave::BraveRequestInfo::kInvalidResourceType:
    default:
      break;
  }
  return filter_option;
}

}  // namespace

namespace brave_shields {

AdBlockBaseService::AdBlockBaseService()
    : BaseBraveShieldsService(),
      ad_block_client_(new AdBlockClient()),
      weak_factory_(this) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

AdBlockBaseService::~AdBlockBaseService() {
  Cleanup();
}

void AdBlockBaseService::Cleanup() {
  ad_block_client_.reset();
}

bool AdBlockBaseService::ShouldStartRequest(const GURL& url,
    content::ResourceType resource_type, const std::string& tab_host,
    bool* did_match_exception, bool* cancel_request_explicitly) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  FilterOption current_option = ResourceTypeToFilterOption(resource_type);

  // Determine third-party here so the library doesn't need to figure it out.
  // CreateFromNormalizedTuple is needed because SameDomainOrHost needs
  // a URL or origin and not a string to a host name.
  if (SameDomainOrHost(url, url::Origin::CreateFromNormalizedTuple(
        "https", tab_host.c_str(), 80), INCLUDE_PRIVATE_REGISTRIES)) {
    current_option = static_cast<FilterOption>(
        current_option | FONotThirdParty);
  } else {
    current_option = static_cast<FilterOption>(current_option | FOThirdParty);
  }

  Filter* matching_filter = nullptr;
  Filter* matching_exception_filter = nullptr;
  if (ad_block_client_->matches(url.spec().c_str(),
        current_option, tab_host.c_str(), &matching_filter,
        &matching_exception_filter)) {
    if (matching_filter && cancel_request_explicitly &&
        (matching_filter->filterOption & FOExplicitCancel)) {
      *cancel_request_explicitly = true;
    }
    // We'd only possibly match an exception filter if we're returning true.
    *did_match_exception = false;
    // LOG(ERROR) << "AdBlockBaseService::ShouldStartRequest(), host: "
    //  << tab_host
    //  << ", resource type: " << resource_type
    //  << ", url.spec(): " << url.spec();
    return false;
  }

  if (did_match_exception) {
    *did_match_exception = !!matching_exception_filter;
  }

  return true;
}

void AdBlockBaseService::EnableTag(const std::string& tag, bool enabled) {
  GetTaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(&AdBlockBaseService::EnableTagOnFileTaskRunner,
                     base::Unretained(this), tag, enabled));
}

void AdBlockBaseService::EnableTagOnFileTaskRunner(
    std::string tag, bool enabled) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (enabled) {
    ad_block_client_->addTag(tag);
  } else {
    ad_block_client_->removeTag(tag);
  }
}

void AdBlockBaseService::GetDATFileData(const base::FilePath& dat_file_path) {
  GetTaskRunner()->PostTaskAndReply(
      FROM_HERE,
      base::Bind(&brave_shields::GetDATFileData, dat_file_path, &buffer_),
      base::Bind(&AdBlockBaseService::OnDATFileDataReady,
                 weak_factory_.GetWeakPtr()));
}

void AdBlockBaseService::OnDATFileDataReady() {
  if (buffer_.empty()) {
    LOG(ERROR) << "Could not obtain ad block data";
    return;
  }
  if (!ad_block_client_->deserialize(
      reinterpret_cast<char*>(&buffer_.front()))) {
    ad_block_client_.reset(new AdBlockClient());
    LOG(ERROR) << "Failed to deserialize ad block data";
    return;
  }
}

bool AdBlockBaseService::Init() {
  return true;
}

AdBlockClient* AdBlockBaseService::GetAdBlockClientForTest() {
  return ad_block_client_.get();
}

///////////////////////////////////////////////////////////////////////////////

}  // namespace brave_shields
