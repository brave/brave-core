/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/webui/webui_util.h"

#define SetupWebUIDataSource SetupWebUIDataSource_ChromiumImpl
#include "../../../../../../chrome/browser/ui/webui/webui_util.cc"
#undef SetupWebUIDataSource

namespace webui {

namespace {
constexpr char kBraveCSP[] =
    "script-src chrome://resources chrome://brave-resources chrome://test "
    "'self';";
}  // namespace

void SetupWebUIDataSource(content::WebUIDataSource* source,
                          base::span<const webui::ResourcePath> resources,
                          int default_resource) {
  SetupWebUIDataSource_ChromiumImpl(source, resources, default_resource);
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc, kBraveCSP);
}

}  // namespace webui
