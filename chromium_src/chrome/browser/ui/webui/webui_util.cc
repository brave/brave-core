/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/webui/webui_util.h"

#include "chrome/common/buildflags.h"

#define SetupWebUIDataSource SetupWebUIDataSource_ChromiumImpl
#if BUILDFLAG(OPTIMIZE_WEBUI)
#define SetupBundledWebUIDataSource SetupBundledWebUIDataSource_ChromiumImpl
#endif

#include "../../../../../../chrome/browser/ui/webui/webui_util.cc"

#undef SetupWebUIDataSource
#if BUILDFLAG(OPTIMIZE_WEBUI)
#undef SetupBundledWebUIDataSource
#endif

namespace webui {

namespace {
constexpr char kBraveCSP[] =
    "script-src chrome://resources chrome://brave-resources chrome://test "
    "'self';";
}  // namespace

void SetupWebUIDataSource(content::WebUIDataSource* source,
                          base::span<const GritResourceMap> resources,
                          const std::string& generated_path,
                          int default_resource) {
  SetupWebUIDataSource_ChromiumImpl(source, resources, generated_path,
                                    default_resource);
  source->OverrideContentSecurityPolicyScriptSrc(kBraveCSP);
}

#if BUILDFLAG(OPTIMIZE_WEBUI)
void SetupBundledWebUIDataSource(content::WebUIDataSource* source,
                                 base::StringPiece bundled_path,
                                 int bundle,
                                 int default_resource) {
  SetupBundledWebUIDataSource_ChromiumImpl(source, bundled_path, bundle,
                                           default_resource);
  source->OverrideContentSecurityPolicyScriptSrc(kBraveCSP);
}
#endif

}  // namespace webui
