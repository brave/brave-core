// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BROWSER_NAVIGATOR_PARAMS_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BROWSER_NAVIGATOR_PARAMS_H_

#include "brave/components/containers/buildflags/buildflags.h"
#include "content/public/browser/storage_partition_config.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#define blob_url_loader_factory \
  blob_url_loader_factory;      \
  std::optional<content::StoragePartitionConfig> storage_partition_config
#endif

#include <chrome/browser/ui/browser_navigator_params.h>  // IWYU pragma: export

#if BUILDFLAG(ENABLE_CONTAINERS)
#undef blob_url_loader_factory
#endif

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BROWSER_NAVIGATOR_PARAMS_H_
