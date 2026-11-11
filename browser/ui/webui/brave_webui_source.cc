/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_webui_source.h"

#include <string_view>

#include "base/containers/span.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "brave/components/webui/webui_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_ui_data_source.h"
#include "services/network/public/mojom/content_security_policy.mojom.h"
#include "ui/base/webui/resource_path.h"

namespace {

void CustomizeWebUIHTMLSource(content::WebUI* web_ui,
                              std::string_view name,
                              content::WebUIDataSource* source) {
  source->AddResourcePaths(brave::GetWebUIResources(name));
  source->AddLocalizedStrings(brave::GetWebUILocalizedStrings(name));
}

content::WebUIDataSource* CreateWebUIDataSource(
    content::WebUI* web_ui,
    std::string_view name,
    base::span<const webui::ResourcePath> resource_paths,
    int html_resource_id) {
  content::WebUIDataSource* source = content::WebUIDataSource::CreateAndAdd(
      Profile::FromWebUI(web_ui), std::string(name));

  // Create a trusted-types policy - note: Brave's default trusted types are
  // applied in OverrideContentSecurityPolicy, so this is empty to clear the
  // trusted-types policies from upstream we don't need.
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::TrustedTypes, "trusted-types;");

  source->UseStringsJs();
  source->SetDefaultResource(html_resource_id);
  source->AddResourcePaths(resource_paths);
  CustomizeWebUIHTMLSource(web_ui, name, source);
  return source;
}

}  // namespace

content::WebUIDataSource* CreateAndAddWebUIDataSource(
    content::WebUI* web_ui,
    std::string_view name,
    base::span<const webui::ResourcePath> resource_paths,
    int html_resource_id) {
  content::WebUIDataSource* data_source =
      CreateWebUIDataSource(web_ui, name, resource_paths, html_resource_id);
  return data_source;
}
