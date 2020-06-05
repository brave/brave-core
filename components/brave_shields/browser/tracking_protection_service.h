/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_TRACKING_PROTECTION_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_TRACKING_PROTECTION_SERVICE_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/sequenced_task_runner.h"
#include "base/synchronization/lock.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "brave/components/brave_component_updater/browser/local_data_files_observer.h"
#include "brave/components/brave_shields/browser/buildflags/buildflags.h"  // For STP
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom-shared.h"
#include "url/gurl.h"

class HostContentSettingsMap;
class TrackingProtectionServiceTest;

using brave_component_updater::LocalDataFilesObserver;
using brave_component_updater::LocalDataFilesService;

namespace brave_shields {

// The brave shields service in charge of tracking protection and init.
class TrackingProtectionService : public LocalDataFilesObserver {
 public:
  explicit TrackingProtectionService(
      LocalDataFilesService* local_data_files_service);
  ~TrackingProtectionService() override;

  static bool IsSmartTrackingProtectionEnabled();

  bool ShouldStartRequest(const GURL& spec,
                          blink::mojom::ResourceType resource_type,
                          const std::string& tab_host,
                          bool* matching_exception_filter,
                          bool* cancel_request_explicitly);

  // implementation of LocalDataFilesObserver
  void OnComponentReady(const std::string& component_id,
                        const base::FilePath& install_dir,
                        const std::string& manifest) override;

  // ShouldStoreState returns false if the Storage API is being invoked
  // by a site in the tracker list, and tracking protection is enabled for the
  // site that initiated the redirect tracking
  bool ShouldStoreState(HostContentSettingsMap* map,
                        int render_process_id,
                        int render_frame_id,
                        const GURL& top_origin_url,
                        const GURL& origin_url) const;

#if BUILDFLAG(BRAVE_STP_ENABLED)
  void SetStartingSiteForRenderFrame(GURL starting_site,
                                     int render_process_id,
                                     int render_frame_id);
  GURL GetStartingSiteForRenderFrame(int render_process_id,
                                     int render_frame_id) const;

  void DeleteRenderFrameKey(int render_process_id, int render_frame_id);
  void ModifyRenderFrameKey(int old_render_process_id,
                            int old_render_frame_id,
                            int new_render_process_id,
                            int new_render_frame_id);
#endif

 protected:
#if BUILDFLAG(BRAVE_STP_ENABLED)
  // ParseStorageTrackersData parses the storage trackers list provided by
  // the offline-crawler
  void OnGetSTPDATFileData(std::string contents);
  void UpdateFirstPartyStorageTrackers(std::vector<std::string>);

  // For Smart Tracking Protection, we need to keep track of the starting site
  // that initiated the redirects. We use RenderFrameIdKey to determine the
  // starting site for a given render frame host.
  struct RenderFrameIdKey {
    RenderFrameIdKey();
    RenderFrameIdKey(int render_process_id, int frame_routing_id);

    int render_process_id;
    int frame_routing_id;

    bool operator<(const RenderFrameIdKey& other) const;
    bool operator==(const RenderFrameIdKey& other) const;
  };
#endif

 private:
#if BUILDFLAG(BRAVE_STP_ENABLED)
  base::flat_set<std::string> first_party_storage_trackers_;
  std::map<RenderFrameIdKey, GURL> render_frame_key_to_starting_site_url;
#endif

  std::vector<std::string> third_party_base_hosts_;
  std::map<std::string, std::vector<std::string>> third_party_hosts_cache_;
  base::Lock third_party_hosts_lock_;

  base::WeakPtrFactory<TrackingProtectionService> weak_factory_;
  base::WeakPtrFactory<TrackingProtectionService> weak_factory_io_thread_;
  DISALLOW_COPY_AND_ASSIGN(TrackingProtectionService);
};

// Creates the TrackingProtectionService
std::unique_ptr<TrackingProtectionService> TrackingProtectionServiceFactory(
    LocalDataFilesService* local_data_files_service);

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_TRACKING_PROTECTION_SERVICE_H_
