/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/content_settings/host_content_settings_map_factory.h"

#include "brave/components/content_settings/core/browser/remote_list_provider.h"
#include "build/buildflag.h"
#include "chrome/browser/supervised_user/supervised_user_settings_service_factory.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/simple_keyed_service_factory.h"
#include "extensions/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "extensions/browser/browser_context_keyed_api_factory.h"
#endif

#define BuildServiceInstanceFor BuildServiceInstanceFor_ChromiumImpl

#include "src/chrome/browser/content_settings/host_content_settings_map_factory.cc"

#undef BuildServiceInstanceFor

scoped_refptr<RefcountedKeyedService>
HostContentSettingsMapFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  scoped_refptr<RefcountedKeyedService> settings_map =
      BuildServiceInstanceFor_ChromiumImpl(context);
  auto remote_list_provider_ptr =
      std::make_unique<content_settings::RemoteListProvider>();
  static_cast<HostContentSettingsMap*>(settings_map.get())
      ->RegisterProvider(ProviderType::kRemoteListProvider,
                         std::move(remote_list_provider_ptr));
  return settings_map;
}
