// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/search_query_metrics/search_query_metrics_utils.h"
#include "chrome/browser/shell_integration.h"

namespace metrics::utils {

bool IsDefaultBrowser() {
#if !BUILDFLAG(IS_LINUX)
  shell_integration::DefaultWebClientState state =
      shell_integration::GetDefaultBrowser();
  return state == shell_integration::IS_DEFAULT ||
         state == shell_integration::OTHER_MODE_IS_DEFAULT;
#else
  // Linux support for is default browser check increases code complexity, so we
  // just return false for now and decide if we need to support it later.
  return false;
#endif
}

}  // namespace metrics::utils
