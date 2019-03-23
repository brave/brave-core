/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_TRACKING_PROTECTION_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_TRACKING_PROTECTION_SERVICE_H_

#include <stdint.h>

#include <map>
#include <memory>
// TODO(brave): <mutex> is an unapproved C++11 header
#include <mutex>  // NOLINT
#include <string>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "base/sequenced_task_runner.h"
#include "brave/components/brave_shields/browser/base_local_data_files_observer.h"
#include "brave/components/brave_shields/browser/buildflags/buildflags.h"  // For STP
#include "brave/components/brave_shields/browser/dat_file_util.h"
#include "content/public/common/resource_type.h"
#include "url/gurl.h"

class CTPParser;
class HostContentSettingsMap;
class TrackingProtectionServiceTest;

namespace content_settings {
  class BraveCookieSettings;
}

namespace brave_shields {

// The brave shields service in charge of tracking protection and init.
class TrackingProtectionService : public BaseLocalDataFilesObserver {
 public:
  TrackingProtectionService();
  ~TrackingProtectionService() override;

  bool ShouldStartRequest(const GURL& spec,
                          content::ResourceType resource_type,
                          const std::string& tab_host,
                          bool* matching_exception_filter,
                          bool* cancel_request_explicitly);

  bool ShouldStoreState(content_settings::BraveCookieSettings* settings,
                        HostContentSettingsMap* map,
                        int render_process_id,
                        int render_frame_id,
                        const GURL& url,
                        const GURL& first_party_url,
                        const GURL& tab_url) const;

  scoped_refptr<base::SequencedTaskRunner> GetTaskRunner();

  // implementation of BaseLocalDataFilesObserver
  void OnComponentReady(const std::string& component_id,
                        const base::FilePath& install_dir,
                        const std::string& manifest) override;

#if BUILDFLAG(BRAVE_STP_ENABLED)
  // ShouldStoreState returns false if the Storage API is being invoked
  // by a site in the tracker list, and tracking protection is enabled for the
  // site that initiated the redirect tracking
  bool ShouldStoreState(HostContentSettingsMap* map,
                        int render_process_id,
                        int render_frame_id,
                        const GURL& top_origin_url,
                        const GURL& origin_url) const;

  void SetStartingSiteForRenderFrame(GURL starting_site, int render_process_id,
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
  void ParseStorageTrackersData();

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
  void OnDATFileDataReady();
  std::vector<std::string> GetThirdPartyHosts(const std::string& base_host);

#if BUILDFLAG(BRAVE_STP_ENABLED)
  base::flat_set<std::string> first_party_storage_trackers_;
  std::map<RenderFrameIdKey, GURL> render_frame_key_to_starting_site_url;

  brave_shields::DATFileDataBuffer storage_trackers_buffer_;
#endif
  brave_shields::DATFileDataBuffer buffer_;

  std::unique_ptr<CTPParser> tracking_protection_client_;
  std::vector<std::string> third_party_base_hosts_;
  std::map<std::string, std::vector<std::string>> third_party_hosts_cache_;
  std::mutex third_party_hosts_mutex_;

  SEQUENCE_CHECKER(sequence_checker_);
  base::WeakPtrFactory<TrackingProtectionService> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(TrackingProtectionService);
};

// Creates the TrackingProtectionService
std::unique_ptr<TrackingProtectionService> TrackingProtectionServiceFactory();

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_TRACKING_PROTECTION_SERVICE_H_
