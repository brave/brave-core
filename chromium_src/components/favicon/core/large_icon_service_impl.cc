// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Google's Favicon Server (t0.gstatic.com/faviconV2) returns 404 specifically
// for www.google.com. Other Google regional domains (www.google.ca,
// www.google.co.uk, etc.) work fine with or without the www. prefix, and all
// non-Google domains also work regardless of www. prefix. Only www.google.com
// is broken. Chrome works around this by bundling Google's icon from a private
// repo (gated by ENABLE_BUILTIN_SEARCH_PROVIDER_ASSETS), but Brave can't use
// that approach. Fix by rewriting www.google.com to google.com before the
// request.
//
// This primarily affects Android, where the Quick Search Engines bar and Search
// Engine Settings call
// GetLargeIconOrFallbackStyleFromGoogleServerSkippingLocal- Cache() directly
// with no built-in icon fallback. Desktop is unaffected because Brave overrides
// GetSuperGIcon() (in chromium_src/chrome/browser/ui/
// omnibox/omnibox_edit_model.cc) to return a local vector icon, and
// the favicon cache is populated from browsing history.
// See https://github.com/brave/brave-browser/issues/47271

// Include the header first so Replacements::ClearRef() in url_canon.h
// is fully defined before the macro is active.
#include "components/favicon/core/large_icon_service_impl.h"

#define ClearRef()                           \
  ClearRef();                                \
  if (page_url.host() == "www.google.com") { \
    replacements.SetHostStr("google.com");   \
  }

#include <components/favicon/core/large_icon_service_impl.cc>

#undef ClearRef
