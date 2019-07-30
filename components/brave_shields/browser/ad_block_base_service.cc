/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/ad_block_base_service.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/post_task.h"
#include "brave/browser/net/url_context.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/vendor/adblock_rust_ffi/src/wrapper.hpp"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/origin.h"

using brave_component_updater::BraveComponent;
using content::BrowserThread;
using namespace net::registry_controlled_domains;  // NOLINT

namespace {

std::string ResourceTypeToString(content::ResourceType resource_type) {
  std::string filter_option = "";
  switch (resource_type) {
    // top level page
    case content::ResourceType::kMainFrame:
      filter_option = "main_frame";
      break;
    // frame or iframe
    case content::ResourceType::kSubFrame:
      filter_option = "sub_frame";
      break;
    // a CSS stylesheet
    case content::ResourceType::kStylesheet:
      filter_option = "stylesheet";
      break;
    // an external script
    case content::ResourceType::kScript:
      filter_option = "script";
      break;
    // an image (jpg/gif/png/etc)
    case content::ResourceType::kFavicon:
    case content::ResourceType::kImage:
      filter_option = "image";
      break;
    // a font
    case content::ResourceType::kFontResource:
      filter_option = "font";
      break;
    // an "other" subresource.
    case content::ResourceType::kSubResource:
      filter_option = "other";
      break;
    // an object (or embed) tag for a plugin.
    case content::ResourceType::kObject:
      filter_option = "object";
      break;
    // a media resource.
    case content::ResourceType::kMedia:
      filter_option = "media";
      break;
    // a XMLHttpRequest
    case content::ResourceType::kXhr:
      filter_option = "xhr";
      break;
    // a ping request for <a ping>/sendBeacon.
    case content::ResourceType::kPing:
      filter_option = "ping";
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
    default:
      break;
  }
  return filter_option;
}

}  // namespace

namespace brave_shields {

AdBlockBaseService::AdBlockBaseService(BraveComponent::Delegate* delegate)
    : BaseBraveShieldsService(delegate),
      ad_block_client_(new adblock::Engine()),
      weak_factory_(this),
      weak_factory_io_thread_(this) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

AdBlockBaseService::~AdBlockBaseService() {
  Cleanup();
}

void AdBlockBaseService::Cleanup() {
  BrowserThread::DeleteSoon(
      BrowserThread::IO, FROM_HERE, ad_block_client_.release());
}

bool AdBlockBaseService::ShouldStartRequest(const GURL& url,
    content::ResourceType resource_type, const std::string& tab_host,
    bool* did_match_exception, bool* cancel_request_explicitly,
    const BlockDecision** block_decision) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  // Determine third-party here so the library doesn't need to figure it out.
  // CreateFromNormalizedTuple is needed because SameDomainOrHost needs
  // a URL or origin and not a string to a host name.
  bool is_third_party = !SameDomainOrHost(url,
      url::Origin::CreateFromNormalizedTuple("https", tab_host.c_str(), 80),
      INCLUDE_PRIVATE_REGISTRIES);
  bool explicit_cancel;
  bool saved_from_exception;
  std::string filter;
  // TODO(bbondy): Use redirect if it is provided.
  std::string redirect;
  if (ad_block_client_->matches(url.spec(), url.host(),
        tab_host, is_third_party, ResourceTypeToString(resource_type),
        &explicit_cancel, &saved_from_exception, &filter, &redirect)) {
    if (cancel_request_explicitly) {
      *cancel_request_explicitly = explicit_cancel;
    }
    // We'd only possibly match an exception filter if we're returning true.
    if (did_match_exception) {
      *did_match_exception = false;
    }
    if (block_decision) {
      *block_decision = new AdBlockDecision(filter);
    }
    // LOG(ERROR) << "AdBlockBaseService::ShouldStartRequest(), host: "
    //  << tab_host
    //  << ", resource type: " << resource_type
    //  << ", url.spec(): " << url.spec();
    return false;
  }

  if (did_match_exception) {
    *did_match_exception = saved_from_exception;
  }

  return true;
}

void AdBlockBaseService::EnableTag(const std::string& tag, bool enabled) {
  base::PostTaskWithTraits(
      FROM_HERE, {BrowserThread::IO},
      base::BindOnce(&AdBlockBaseService::EnableTagOnIOThread,
                     weak_factory_io_thread_.GetWeakPtr(),
                     tag,
                     enabled));
}

void AdBlockBaseService::EnableTagOnIOThread(
    const std::string& tag, bool enabled) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (enabled) {
    ad_block_client_->addTag(tag);
    tags_.push_back(tag);
  } else {
    ad_block_client_->removeTag(tag);
    std::vector<std::string>::iterator it =
        std::find(tags_.begin(), tags_.end(), tag);
    if (it != tags_.end()) {
      tags_.erase(it);
    }
  }
}

bool AdBlockBaseService::TagExists(const std::string& tag) {
  return std::find(tags_.begin(), tags_.end(), tag) != tags_.end();
}

void AdBlockBaseService::GetDATFileData(const base::FilePath& dat_file_path) {
  base::PostTaskAndReplyWithResult(
      GetTaskRunner().get(),
      FROM_HERE,
      base::BindOnce(
          &brave_component_updater::LoadDATFileData<adblock::Engine>,
          dat_file_path),
      base::BindOnce(&AdBlockBaseService::OnGetDATFileData,
                     weak_factory_.GetWeakPtr()));
}

void AdBlockBaseService::OnGetDATFileData(GetDATFileDataResult result) {
  if (result.second.empty()) {
    LOG(ERROR) << "Could not obtain ad block data";
    return;
  }
  if (!result.first.get()) {
    LOG(ERROR) << "Failed to deserialize ad block data";
    return;
  }

  base::PostTaskWithTraits(
      FROM_HERE, {BrowserThread::IO},
      base::BindOnce(&AdBlockBaseService::UpdateAdBlockClient,
                     weak_factory_io_thread_.GetWeakPtr(),
                     std::move(result.first),
                     std::move(result.second)));
}

void AdBlockBaseService::UpdateAdBlockClient(
    std::unique_ptr<adblock::Engine> ad_block_client,
    brave_component_updater::DATFileDataBuffer buffer) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  ad_block_client_ = std::move(ad_block_client);
  buffer_ = std::move(buffer);
  AddKnownTagsToAdBlockInstance();
}

void AdBlockBaseService::AddKnownTagsToAdBlockInstance() {
  std::for_each(tags_.begin(), tags_.end(), [&](const std::string tag) {
    ad_block_client_->addTag(tag);
  });
}


bool AdBlockBaseService::Init() {
  return true;
}

void AdBlockBaseService::ResetForTest(const std::string& rules) {
  // This is temporary until adblock-rust supports incrementally adding
  // filter rules to an existing instance. At which point the hack below
  // will dissapear.
  DETACH_FROM_SEQUENCE(sequence_checker_);
  ad_block_client_.reset(new adblock::Engine(rules));
  AddKnownTagsToAdBlockInstance();
}

///////////////////////////////////////////////////////////////////////////////

}  // namespace brave_shields
