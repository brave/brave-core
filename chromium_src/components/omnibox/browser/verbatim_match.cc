// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "components/omnibox/browser/verbatim_match.h"

#include "content/public/common/url_constants.h"

#if BUILDFLAG(IS_ANDROID)
#define kChromeUIScheme kChromeUIScheme, content::kBraveUIScheme
#endif  // BUILDFLAG(IS_ANDROID)
#include "src/components/omnibox/browser/verbatim_match.cc"
#if BUILDFLAG(IS_ANDROID)
#undef kChromeUIScheme
#endif  // BUILDFLAG(IS_ANDROID)
