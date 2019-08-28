/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/translate/buildflags/buildflags.h"

// Only allows TranslateURLFetcher::Request to work when using go-translate.
#if BUILDFLAG(ENABLE_BRAVE_TRANSLATE_GO)
#define BRAVE_REQUEST
#else
#define BRAVE_REQUEST return false;
#endif

#include "../../../../../../components/translate/core/browser/translate_url_fetcher.cc"
#undef BRAVE_REQUEST
