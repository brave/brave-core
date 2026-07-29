/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/download/brave_download_manager_delegate.h"

#define BRAVE_CREATE_DOWNLOAD_MANAGER_DELEGATE \
  manager_delegate_ = std::make_unique<BraveDownloadManagerDelegate>(profile_);
#include <chrome/browser/download/download_core_service_impl.cc>
#undef BRAVE_CREATE_DOWNLOAD_MANAGER_DELEGATE
