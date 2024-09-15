/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/sync/sync_service_factory.h"

#include "base/command_line.h"
#include "brave/browser/sync/brave_sync_service_impl_delegate.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "brave/components/sync/service/brave_sync_service_impl.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/sync/device_info_sync_service_factory.h"
#include "components/sync/base/command_line_switches.h"

// Below includes are just to prevent redefining of
// |BuildServiceInstanceForBrowserContext|.
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/commerce/product_specifications/product_specifications_service_factory.h"
#include "chrome/browser/consent_auditor/consent_auditor_factory.h"
#include "chrome/browser/favicon/favicon_service_factory.h"
#include "chrome/browser/metrics/variations/google_groups_manager_factory.h"
#include "chrome/browser/password_manager/password_receiver_service_factory.h"
#include "chrome/browser/password_manager/password_sender_service_factory.h"
#include "chrome/browser/plus_addresses/plus_address_setting_service_factory.h"
#include "chrome/browser/power_bookmarks/power_bookmark_service_factory.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/security_events/security_event_recorder_factory.h"
#include "chrome/browser/sharing/sharing_message_bridge_factory.h"
#include "chrome/browser/spellchecker/spellcheck_factory.h"
#include "chrome/browser/ui/tabs/saved_tab_groups/saved_tab_group_service_factory.h"
#include "chrome/browser/web_applications/web_app_provider_factory.h"
#include "chrome/browser/webauthn/passkey_model_factory.h"
#include "chrome/browser/webdata_services/web_data_service_factory.h"

#define BRAVE_BUILD_SERVICE_INSTANCE_FOR                        \
  std::make_unique<syncer::BraveSyncServiceImpl>(               \
      std::move(init_params),                                   \
      std::make_unique<syncer::BraveSyncServiceImplDelegate>(   \
          DeviceInfoSyncServiceFactory::GetForProfile(profile), \
          HistoryServiceFactory::GetForProfile(                 \
              profile, ServiceAccessType::IMPLICIT_ACCESS)));

#define BuildServiceInstanceForBrowserContext \
  BuildServiceInstanceForBrowserContext_ChromiumImpl

#include "src/chrome/browser/sync/sync_service_factory.cc"

#undef BuildServiceInstanceForBrowserContext
#undef BRAVE_BUILD_SERVICE_INSTANCE_FOR

std::unique_ptr<KeyedService>
SyncServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  auto* command_line = base::CommandLine::ForCurrentProcess();
  Profile* profile = Profile::FromBrowserContext(context);

  // Pass the custom sync service URL to the sync service if it's not managed.
  if (!profile->GetPrefs()->IsManagedPreference(
          brave_sync::kCustomSyncServiceUrl)) {
    if (!command_line->HasSwitch(syncer::kSyncServiceURL) ||
        command_line->GetSwitchValueASCII(syncer::kSyncServiceURL).empty()) {
      command_line->AppendSwitchASCII(
          syncer::kSyncServiceURL,
          profile->GetPrefs()
              ->GetString(brave_sync::kCustomSyncServiceUrl)
              .c_str());
    }
  }

  return BuildServiceInstanceForBrowserContext_ChromiumImpl(context);
}
