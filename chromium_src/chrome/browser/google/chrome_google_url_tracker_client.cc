/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// This include is required here since chrome_google_url_tracker_client.cc
// incldues it. We don't want replacements to happen though.
// The header guard will ensure this even know the .cc includes it.
#include "chrome/common/chrome_switches.h"

// Required to declare kDisableChromeGoogleURLTrackingClient
#include "brave/common/brave_switches.h"

// The ChromeGooglURLTrackerClient will disable itself when the kDisableBackgroundNetworking
// switch is set.  Since several other services also disable themeselves when that is set,
// we rename the switch that is used to something we define.
#define kDisableBackgroundNetworking kDisableChromeGoogleURLTrackingClient
#include "../../../../../chrome/browser/google/chrome_google_url_tracker_client.cc"
#undef kDisableBackgroundNetworking
