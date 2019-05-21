/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/tracking_protection_service.h"

#include <utility>

#include "base/bind.h"
#include "base/task/post_task.h"
#include "base/task_runner_util.h"
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"
#include "brave/components/content_settings/core/browser/brave_cookie_settings.h"
#include "brave/vendor/tracking-protection/TPParser.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"

#if BUILDFLAG(BRAVE_STP_ENABLED)
#include "base/strings/string_split.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/browser/tracking_protection_helper.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#endif

using content::BrowserThread;
using content_settings::BraveCookieSettings;

namespace brave_shields {

const char kDatFileVersion[] = "1";
const char kNavigationTrackersFile[] = "TrackingProtection.dat";

#if BUILDFLAG(BRAVE_STP_ENABLED)
const char kStorageTrackersFile[] = "StorageTrackingProtection.dat";
#endif

const int kThirdPartyHostsCacheSize = 20;

TrackingProtectionService::TrackingProtectionService(
    LocalDataFilesService* local_data_files_service)
    : LocalDataFilesObserver(local_data_files_service),
      tracking_protection_client_(new CTPParser()),
      weak_factory_(this),
      weak_factory_io_thread_(this) {
}

TrackingProtectionService::~TrackingProtectionService() {
  BrowserThread::DeleteSoon(
      BrowserThread::IO, FROM_HERE, tracking_protection_client_.release());
}

#if BUILDFLAG(BRAVE_STP_ENABLED)
TrackingProtectionService::RenderFrameIdKey::RenderFrameIdKey()
    : render_process_id(content::ChildProcessHost::kInvalidUniqueID),
      frame_routing_id(MSG_ROUTING_NONE) {}

TrackingProtectionService::RenderFrameIdKey::RenderFrameIdKey(
    int render_process_id,
    int frame_routing_id)
    : render_process_id(render_process_id),
      frame_routing_id(frame_routing_id) {}

bool TrackingProtectionService::RenderFrameIdKey::operator<(
    const RenderFrameIdKey& other) const {
  return std::tie(render_process_id, frame_routing_id) <
         std::tie(other.render_process_id, other.frame_routing_id);
}

bool TrackingProtectionService::RenderFrameIdKey::operator==(
    const RenderFrameIdKey& other) const {
  return render_process_id == other.render_process_id &&
         frame_routing_id == other.frame_routing_id;
}

void TrackingProtectionService::SetStartingSiteForRenderFrame(
    GURL starting_site,
    int render_process_id,
    int render_frame_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  const RenderFrameIdKey key(render_process_id, render_frame_id);
  render_frame_key_to_starting_site_url[key] = starting_site;
}

GURL TrackingProtectionService::GetStartingSiteForRenderFrame(
    int render_process_id,
    int render_frame_id) const {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  const RenderFrameIdKey key(render_process_id, render_frame_id);
  auto iter = render_frame_key_to_starting_site_url.find(key);
  if (iter != render_frame_key_to_starting_site_url.end()) {
    return iter->second;
  }
  return {};
}

void TrackingProtectionService::ModifyRenderFrameKey(int old_render_process_id,
                                                     int old_render_frame_id,
                                                     int new_render_process_id,
                                                     int new_render_frame_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  const RenderFrameIdKey old_key(old_render_process_id, old_render_frame_id);
  auto iter = render_frame_key_to_starting_site_url.find(old_key);
  if (iter != render_frame_key_to_starting_site_url.end()) {
    const RenderFrameIdKey new_key(new_render_process_id, new_render_frame_id);
    render_frame_key_to_starting_site_url.insert(
        std::pair<RenderFrameIdKey, GURL>(new_key, iter->second));
    render_frame_key_to_starting_site_url.erase(old_key);
  }
}

void TrackingProtectionService::DeleteRenderFrameKey(int render_process_id,
                                                     int render_frame_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  const RenderFrameIdKey key(render_process_id, render_frame_id);
  render_frame_key_to_starting_site_url.erase(key);
}

bool TrackingProtectionService::ShouldStoreState(HostContentSettingsMap* map,
                                                 int render_process_id,
                                                 int render_frame_id,
                                                 const GURL& top_origin_url,
                                                 const GURL& origin_url) const {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!TrackingProtectionHelper::IsSmartTrackingProtectionEnabled()) {
    return true;
  }

  if (first_party_storage_trackers_.empty()) {
    LOG(INFO) << "First party storage trackers list is empty";
    return true;
  }

  const std::string host = origin_url.host();
  const GURL starting_site =
      GetStartingSiteForRenderFrame(render_process_id, render_frame_id);

  // If starting host is the current host, user-interaction has happened
  // so we allow storage
  if (starting_site.host() == host) {
    return true;
  }

  const bool allow_brave_shields =
      starting_site.is_empty()
          ? false
          : IsAllowContentSetting(map, starting_site, GURL(),
                                  CONTENT_SETTINGS_TYPE_PLUGINS,
                                  brave_shields::kBraveShields);

  if (!allow_brave_shields) {
    return true;
  }

  const bool allow_trackers =
      starting_site.is_empty()
          ? true
          : IsAllowContentSetting(map, starting_site, GURL(),
                                  CONTENT_SETTINGS_TYPE_PLUGINS,
                                  brave_shields::kTrackers);

  if (allow_trackers) {
    return true;
  }

  // deny storage if host is found in the tracker list
  return first_party_storage_trackers_.find(host) ==
         first_party_storage_trackers_.end();
}

void TrackingProtectionService::OnGetSTPDATFileData(std::string contents) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (contents.empty()) {
    LOG(ERROR) << "Could not obtain first party trackers data";
    return;
  }

  std::vector<std::string> storage_trackers =
      base::SplitString(base::StringPiece(contents.data(), contents.size()),
                        ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  if (storage_trackers.empty()) {
    LOG(ERROR) << "No first party trackers found";
    return;
  }

  base::PostTaskWithTraits(
      FROM_HERE, {BrowserThread::IO},
      base::BindOnce(&TrackingProtectionService::UpdateFirstPartyStorageTrackers,
                     weak_factory_io_thread_.GetWeakPtr(),
                     std::move(storage_trackers)));
}

void TrackingProtectionService::UpdateFirstPartyStorageTrackers(
    std::vector<std::string>) {
  first_party_storage_trackers_ =
      base::flat_set<std::string>(std::move(storage_trackers));
}

#endif

bool TrackingProtectionService::ShouldStoreState(BraveCookieSettings* settings,
                                                 HostContentSettingsMap* map,
                                                 int render_process_id,
                                                 int render_frame_id,
                                                 const GURL& url,
                                                 const GURL& first_party_url,
                                                 const GURL& tab_url) const {
#if BUILDFLAG(BRAVE_STP_ENABLED)
  const bool allow = ShouldStoreState(map, render_process_id, render_frame_id,
                                      url, first_party_url);
  if (!allow) {
    return allow;
  }
#endif

  return settings->IsCookieAccessAllowed(url, first_party_url, tab_url);
}

bool TrackingProtectionService::ShouldStartRequest(
    const GURL& url,
    content::ResourceType resource_type,
    const std::string& tab_host,
    bool* matching_exception_filter,
    bool* cancel_request_explicitly) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  // There are no exceptions in the TP service, but exceptions are
  // combined with brave/ad-block.
  if (matching_exception_filter) {
    *matching_exception_filter = false;
  }
  // Intentionally don't set cancel_request_explicitly
  std::string host = url.host();
  if (!tracking_protection_client_->matchesTracker(tab_host.c_str(),
                                                   host.c_str())) {
    return true;
  }

  std::vector<std::string> hosts(GetThirdPartyHosts(tab_host));
  for (size_t i = 0; i < hosts.size(); i++) {
    if (host == hosts[i] ||
        host.find((std::string) "." + hosts[i]) != std::string::npos) {
      return true;
    }
    size_t iPos = host.find((std::string) "." + hosts[i]);
    if (iPos == std::string::npos) {
      continue;
    }
    if (hosts[i].length() + ((std::string) ".").length() + iPos ==
        host.length()) {
      return true;
    }
  }
  return false;
}

void TrackingProtectionService::OnGetDATFileData(GetDATFileDataResult result) {
  if (result.second.empty()) {
    LOG(ERROR) << "Could not obtain tracking protection data";
    return;
  }
  if (!result.first.get()) {
    LOG(ERROR) << "Failed to deserialize tracking protection data";
    return;
  }

  base::PostTaskWithTraits(
      FROM_HERE, {BrowserThread::IO},
      base::BindOnce(&TrackingProtectionService::UpdateTrackingProtectionClient,
                     weak_factory_io_thread_.GetWeakPtr(),
                     std::move(result.first),
                     std::move(result.second)));
}

void TrackingProtectionService::UpdateTrackingProtectionClient(
    std::unique_ptr<CTPParser> tracking_protection_client,
    brave_component_updater::DATFileDataBuffer buffer) {
  tracking_protection_client_ = std::move(tracking_protection_client);
  buffer_ = std::move(buffer);
}

void TrackingProtectionService::OnComponentReady(
    const std::string& component_id,
    const base::FilePath& install_dir,
    const std::string& manifest) {
  base::FilePath navigation_tracking_protection_path = install_dir
      .AppendASCII(kDatFileVersion)
      .AppendASCII(kNavigationTrackersFile);

  base::PostTaskAndReplyWithResult(
      local_data_files_service()->GetTaskRunner().get(),
      FROM_HERE,
      base::BindOnce(&brave_component_updater::LoadDATFileData<CTPParser>,
                     navigation_tracking_protection_path),
      base::BindOnce(&TrackingProtectionService::OnGetDATFileData,
                     weak_factory_.GetWeakPtr()));

#if BUILDFLAG(BRAVE_STP_ENABLED)
  if (!TrackingProtectionHelper::IsSmartTrackingProtectionEnabled()) {
    return;
  }
  base::FilePath storage_tracking_protection_path = install_dir
      .AppendASCII(kDatFileVersion)
      .AppendASCII(kStorageTrackersFile);

  base::PostTaskAndReplyWithResult(
      local_data_files_service()->GetTaskRunner().get(),
      FROM_HERE,
      base::BindOnce(&brave_component_updater::GetDATFileAsString,
                     storage_tracking_protection_path),
      base::BindOnce(&TrackingProtectionService::OnGetSTPDATFileData,
                     weak_factory_.GetWeakPtr()));
#endif
}

// Ported from Android: net/blockers/blockers_worker.cc
std::vector<std::string> TrackingProtectionService::GetThirdPartyHosts(
    const std::string& base_host) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  {
    base::AutoLock guard(third_party_hosts_lock_);
    std::map<std::string, std::vector<std::string>>::const_iterator iter =
        third_party_hosts_cache_.find(base_host);
    if (third_party_hosts_cache_.end() != iter) {
      if (third_party_base_hosts_.size() != 0 &&
          third_party_base_hosts_[third_party_hosts_cache_.size() - 1] !=
              base_host) {
        for (size_t i = 0; i < third_party_base_hosts_.size(); i++) {
          if (third_party_base_hosts_[i] == base_host) {
            third_party_base_hosts_.erase(third_party_base_hosts_.begin() + i);
            third_party_base_hosts_.push_back(base_host);
            break;
          }
        }
      }
      return iter->second;
    }
  }

  char* thirdPartyHosts =
      tracking_protection_client_->findFirstPartyHosts(base_host.c_str());
  std::vector<std::string> hosts;
  if (nullptr != thirdPartyHosts) {
    std::string strThirdPartyHosts = thirdPartyHosts;
    size_t iPos = strThirdPartyHosts.find(",");
    while (iPos != std::string::npos) {
      std::string thirdParty = strThirdPartyHosts.substr(0, iPos);
      strThirdPartyHosts = strThirdPartyHosts.substr(iPos + 1);
      iPos = strThirdPartyHosts.find(",");
      hosts.push_back(thirdParty);
    }
    if (0 != strThirdPartyHosts.length()) {
      hosts.push_back(strThirdPartyHosts);
    }
    delete []thirdPartyHosts;
  }

  {
    base::AutoLock guard(third_party_hosts_lock_);
    if (third_party_hosts_cache_.size() == kThirdPartyHostsCacheSize &&
        third_party_base_hosts_.size() == kThirdPartyHostsCacheSize) {
      third_party_hosts_cache_.erase(third_party_base_hosts_[0]);
      third_party_base_hosts_.erase(third_party_base_hosts_.begin());
    }
    third_party_base_hosts_.push_back(base_host);
    third_party_hosts_cache_.insert(
        std::pair<std::string, std::vector<std::string>>(base_host, hosts));
  }

  return hosts;
}

///////////////////////////////////////////////////////////////////////////////

std::unique_ptr<TrackingProtectionService> TrackingProtectionServiceFactory(
    LocalDataFilesService* local_data_files_service) {
  std::unique_ptr<TrackingProtectionService> service =
      std::make_unique<TrackingProtectionService>(local_data_files_service);
  return service;
}

}  // namespace brave_shields
