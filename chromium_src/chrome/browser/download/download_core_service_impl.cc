/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>

class ChromeDownloadManagerDelegate;
class Profile;

// Defined in brave/browser/download/brave_download_manager_delegate.cc, and
// declared here so that chrome/browser/download does not depend on Brave
// targets.
std::unique_ptr<ChromeDownloadManagerDelegate>
CreateBraveDownloadManagerDelegate(Profile* profile);

#include <chrome/browser/download/download_core_service_impl.cc>
