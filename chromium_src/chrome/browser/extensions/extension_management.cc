/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Note: BuildServiceInstanceFor is an overriden virtual method, so we need to
// include all the original headers to make sure that we only redefine it here.
#include "chrome/browser/extensions/extension_management.h"
#include "brave/browser/extensions/brave_extension_management.h"
#include "build/chromeos_buildflags.h"
#include "chrome/browser/extensions/extension_management_constants.h"
#include "chrome/browser/extensions/extension_management_internal.h"
#include "chrome/browser/extensions/external_policy_loader.h"
#include "chrome/browser/extensions/external_provider_impl.h"
#include "chrome/browser/extensions/forced_extensions/install_stage_tracker_factory.h"
#include "chrome/browser/extensions/permissions_based_management_policy_provider.h"
#include "chrome/browser/extensions/standard_management_policy_provider.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/extensions/extension_constants.h"
#include "chrome/common/pref_names.h"
#include "components/crx_file/id_util.h"
#include "components/enterprise/browser/reporting/common_pref_names.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "extensions/browser/pref_names.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_urls.h"
#include "extensions/common/manifest_constants.h"
#include "extensions/common/manifest_url_handlers.h"
#include "extensions/common/permissions/api_permission_set.h"
#include "extensions/common/permissions/permission_set.h"
#include "extensions/common/url_pattern.h"
#include "url/gurl.h"

#define BuildServiceInstanceFor BuildServiceInstanceFor_Unused
#include "../../../../../chrome/browser/extensions/extension_management.cc"
#undef BuildServiceInstanceFor

namespace extensions {

KeyedService* ExtensionManagementFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  TRACE_EVENT0("browser,startup",
               "ExtensionManagementFactory::BuildServiceInstanceFor");
  return new BraveExtensionManagement(Profile::FromBrowserContext(context));
}

}  // namespace extensions
