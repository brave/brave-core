/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "content/browser/renderer_host/mixed_content_checker.h"

#include <content/browser/renderer_host/mixed_content_checker.cc>

namespace content {

// static
bool MixedContentChecker::DoesOriginSchemeRestrictMixedContent(
    const url::Origin& origin) {
  // Use the precursor tuple so that opaque origins (e.g. an .onion page
  // sandboxed via `Content-Security-Policy: sandbox`) are still recognized as
  // .onion, mirroring how upstream's DoesOriginSchemeRestrictMixedContent uses
  // GetTupleOrPrecursorTupleIfOpaque(). Reading origin.host()/scheme() directly
  // returns empty strings for opaque origins, which would let mixed content
  // through.
  const url::SchemeHostPort& tuple = origin.GetTupleOrPrecursorTupleIfOpaque();
  if (tuple.host().ends_with(".onion") &&
      (tuple.scheme() == url::kHttpsScheme ||
       tuple.scheme() == url::kHttpScheme || tuple.scheme() == url::kWsScheme ||
       tuple.scheme() == url::kWssScheme)) {
    return true;
  }
  return ::content::DoesOriginSchemeRestrictMixedContent(origin);
}

}  // namespace content
