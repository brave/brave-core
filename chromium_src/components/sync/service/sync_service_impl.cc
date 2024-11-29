/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/command_line.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "brave/components/sync/service/brave_sync_auth_manager.h"
#include "brave/components/sync/service/brave_sync_stopped_reporter.h"
#include "components/prefs/pref_service.h"
#include "components/sync/base/command_line_switches.h"
#include "components/sync/base/sync_util.h"

namespace syncer {

GURL BraveGetSyncServiceURL(const base::CommandLine& command_line,
                            version_info::Channel channel,
                            PrefService* prefs) {
// TODO: Currently, we only handle the custom sync URL via GPO/Settings Page
// on Desktop OSes. However, we should also handle it on Android in the future.
#if !BUILDFLAG(IS_ANDROID)
  if (prefs) {
    std::string value = prefs->GetString(brave_sync::kCustomSyncServiceUrl);
    if (!value.empty()) {
      GURL custom_sync_url(value);
      // Provided URL must be HTTPS.
      if (custom_sync_url.is_valid() &&
          custom_sync_url.SchemeIs(url::kHttpsScheme)) {
        // Allow group policy to override sync service URL.
        // This has a higher priority than the --sync-url command-line param.
        // https://github.com/brave/brave-browser/issues/20431
        if (prefs->IsManagedPreference(brave_sync::kCustomSyncServiceUrl)) {
          DVLOG(2) << "Sync URL specified via GPO: " << value;
          return custom_sync_url;
        } else {
          // If the sync URL is not set via command line.
          if (!command_line.HasSwitch(syncer::kSyncServiceURL) ||
              command_line.GetSwitchValueASCII(syncer::kSyncServiceURL)
                  .empty()) {
            DVLOG(2) << "Sync URL specified via settings: " << value;
            return custom_sync_url;
          }
        }
      } else {
        LOG(WARNING) << "The following sync URL specified via GPO or settings "
                        "page is invalid: "
                     << value;
      }
    }
  }
#endif

  // Default logic.
  // See `GetSyncServiceURL` in `components/sync/base/sync_util.cc`
  return GetSyncServiceURL(command_line, channel);
}

}  // namespace syncer

#define SyncAuthManager BraveSyncAuthManager
#define SyncStoppedReporter BraveSyncStoppedReporter
#define GetSyncServiceURL(...) \
  BraveGetSyncServiceURL(__VA_ARGS__, sync_client_->GetPrefService())

#include "src/components/sync/service/sync_service_impl.cc"

#undef SyncAuthManager
#undef SyncStoppedReporter
#undef GetSyncServiceURL
