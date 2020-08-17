/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "content/public/browser/url_data_source.h"

#define GetContentSecurityPolicy GetContentSecurityPolicy_ChromiumImpl

#include "../../../../../content/public/browser/url_data_source.cc"
#undef GetContentSecurityPolicy

namespace content {

std::string URLDataSource::GetContentSecurityPolicy(
    network::mojom::CSPDirectiveName directive) {
  switch (directive) {
    case network::mojom::CSPDirectiveName::ScriptSrc:
      // Note: Do not add 'unsafe-eval' here. Instead override CSP for the
      // specific pages that need it, see context http://crbug.com/525224.
      return "script-src chrome://resources chrome://brave-resources 'self';";
    default:
      return GetContentSecurityPolicy_ChromiumImpl(directive);
  }
}

}  // namespace content
