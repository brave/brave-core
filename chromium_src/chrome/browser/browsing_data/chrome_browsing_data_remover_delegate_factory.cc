/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Note: BuildServiceInstanceFor is an overriden virtual method, so we need to
// include all the original headers to make sure that we only redefine it here.
#include "chrome/browser/browsing_data/chrome_browsing_data_remover_delegate_factory.h"
#include "build/build_config.h"
#include "chrome/browser/autofill/personal_data_manager_factory.h"
#include "chrome/browser/browsing_data/chrome_browsing_data_remover_delegate.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/data_reduction_proxy/data_reduction_proxy_chrome_settings_factory.h"
#include "chrome/browser/domain_reliability/service_factory.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/history/web_history_service_factory.h"
#include "chrome/browser/password_manager/password_store_factory.h"
#include "chrome/browser/prefetch/no_state_prefetch/prerender_manager_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/sessions/tab_restore_service_factory.h"
#include "chrome/browser/web_data_service_factory.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/browser_context.h"
#include "extensions/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "chrome/browser/extensions/activity_log/activity_log.h"
#include "extensions/browser/extension_prefs_factory.h"
#endif

#if BUILDFLAG(ENABLE_SESSION_SERVICE)
#include "chrome/browser/sessions/session_service_factory.h"
#endif

#if defined(OS_ANDROID)
#include "chrome/browser/android/feed/feed_host_service_factory.h"
#include "chrome/browser/android/feed/v2/feed_service_factory.h"
#include "components/feed/buildflags.h"
#include "components/feed/feed_feature_list.h"
#endif  // defined(OS_ANDROID)

#include "brave/browser/browsing_data/brave_browsing_data_remover_delegate.h"

#define BuildServiceInstanceFor BuildServiceInstanceFor_Unused
#include "../../../../../chrome/browser/browsing_data/chrome_browsing_data_remover_delegate_factory.cc"
#undef BuildServiceInstanceFor

KeyedService* ChromeBrowsingDataRemoverDelegateFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new BraveBrowsingDataRemoverDelegate(context);
}
