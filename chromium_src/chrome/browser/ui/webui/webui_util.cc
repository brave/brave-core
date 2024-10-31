/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/webui/webui_util.h"
#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "content/public/common/url_constants.h"

#define SetupWebUIDataSource SetupWebUIDataSource_ChromiumImpl
#include "src/chrome/browser/ui/webui/webui_util.cc"
#undef SetupWebUIDataSource

namespace webui {

namespace {

// A chrome-untrusted data source's name starts with chrome-untrusted://.
bool IsChromeUntrustedDataSource(content::WebUIDataSource* source) {
  static const base::NoDestructor<std::string> kChromeUntrustedSourceNamePrefix(
      base::StrCat(
          {content::kChromeUIUntrustedScheme, url::kStandardSchemeSeparator}));

  return source->GetSource().starts_with(*kChromeUntrustedSourceNamePrefix);
}

constexpr char kBraveCSP[] =
    "script-src chrome://resources chrome://webui-test "
    "'self';";

constexpr char kBraveUntrustedCSP[] =
    "script-src chrome-untrusted://resources "
    "'self';";
}  // namespace

void SetupWebUIDataSource(content::WebUIDataSource* source,
                          base::span<const webui::ResourcePath> resources,
                          int default_resource) {
  SetupWebUIDataSource_ChromiumImpl(source, resources, default_resource);
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      IsChromeUntrustedDataSource(source) ? kBraveUntrustedCSP : kBraveCSP);
}

}  // namespace webui
