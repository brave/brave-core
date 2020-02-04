/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/tracking_protection_service.h"

#include <utility>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/task/post_task.h"
#include "base/task_runner_util.h"
#include "brave/common/brave_switches.h"
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"
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

namespace brave_shields {

#if BUILDFLAG(BRAVE_STP_ENABLED)
const char kDatFileVersion[] = "1";
const char kStorageTrackersFile[] = "StorageTrackingProtection.dat";
#endif

TrackingProtectionService::TrackingProtectionService(
    LocalDataFilesService* local_data_files_service)
    : LocalDataFilesObserver(local_data_files_service),
      weak_factory_(this),
      weak_factory_io_thread_(this) {
}

TrackingProtectionService::~TrackingProtectionService() {
}

bool TrackingProtectionService::IsSmartTrackingProtectionEnabled() {
#if BUILDFLAG(BRAVE_STP_ENABLED)
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  return command_line.HasSwitch(switches::kEnableSmartTrackingProtection);
#else
  return false;
#endif
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
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
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
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!IsSmartTrackingProtectionEnabled()) {
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

  if (!brave_shields::GetBraveShieldsEnabled(map, starting_site))
    return true;


  if (brave_shields::GetCookieControlType(map, starting_site) !=
      ControlType::BLOCK)
    return true;

  // deny storage if host is found in the tracker list
  return first_party_storage_trackers_.find(host) ==
         first_party_storage_trackers_.end();
}

void TrackingProtectionService::OnGetSTPDATFileData(std::string contents) {
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

  base::PostTask(
      FROM_HERE, {BrowserThread::IO},
      base::BindOnce(
          &TrackingProtectionService::UpdateFirstPartyStorageTrackers,
          weak_factory_io_thread_.GetWeakPtr(), std::move(storage_trackers)));
}

void TrackingProtectionService::UpdateFirstPartyStorageTrackers(
    std::vector<std::string> storage_trackers) {
  first_party_storage_trackers_ =
      base::flat_set<std::string>(std::move(storage_trackers));
}

#else  // !BUILDFLAG(BRAVE_STP_ENABLED)
bool TrackingProtectionService::ShouldStoreState(HostContentSettingsMap* map,
                                                 int render_process_id,
                                                 int render_frame_id,
                                                 const GURL& top_origin_url,
                                                 const GURL& origin_url) const {
  return true;
}
#endif  // BUILDFLAG(BRAVE_STP_ENABLED)

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
  return false;
}

void TrackingProtectionService::OnComponentReady(
    const std::string& component_id,
    const base::FilePath& install_dir,
    const std::string& manifest) {
#if BUILDFLAG(BRAVE_STP_ENABLED)
  if (!IsSmartTrackingProtectionEnabled()) {
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

///////////////////////////////////////////////////////////////////////////////

std::unique_ptr<TrackingProtectionService> TrackingProtectionServiceFactory(
    LocalDataFilesService* local_data_files_service) {
  std::unique_ptr<TrackingProtectionService> service =
      std::make_unique<TrackingProtectionService>(local_data_files_service);
  return service;
}

}  // namespace brave_shields
