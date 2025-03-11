/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "chrome/browser/extensions/chrome_content_verifier_delegate.h"

#include <algorithm>
#include <memory>
#include <set>
#include <vector>

#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/histogram_macros.h"
#include "base/no_destructor.h"
#include "base/strings/string_util.h"
#include "base/syslog_logging.h"
#include "base/version.h"
#include "build/branding_buildflags.h"
#include "build/build_config.h"
#include "build/chromeos_buildflags.h"
#include "chrome/browser/extensions/corrupted_extension_reinstaller.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/install_verifier.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/extensions/extension_constants.h"
#include "extensions/browser/disable_reason.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/management_policy.h"
#include "extensions/browser/pref_types.h"
#include "extensions/common/extension_urls.h"
#include "extensions/common/extensions_client.h"
#include "extensions/common/manifest.h"
#include "extensions/common/manifest_url_handlers.h"
#include "net/base/backoff_entry.h"

// All above headers copied from original chrome_content_verifier_delegate.cc
// are included to prevent below GOOGLE_CHROME_BUILD affect them.

// `VerifyInfo::Mode::ENFORCE_STRICT` is only defaulted for google chrome.
#if defined(OFFICIAL_BUILD)
#undef BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING
#define BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING() (1)
#endif
#include "src/chrome/browser/extensions/chrome_content_verifier_delegate.cc"
#if defined(OFFICIAL_BUILD)
#undef BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING
#endif
