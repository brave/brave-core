/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/basic_ui.h"

#include "brave/browser/ui/webui/brave_webui_source.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/bindings_policy.h"
#include "services/network/public/mojom/content_security_policy.mojom.h"
#include "ui/resources/grit/webui_resources_map.h"

content::WebUIDataSource* CreateBasicUIHTMLSource(
    Profile* profile,
    const std::string& name,
    const GritResourceMap* resource_map,
    size_t resource_map_size,
    int html_resource_id,
    bool disable_trusted_types_csp) {
  content::WebUIDataSource* source =
      content::WebUIDataSource::Create(name);
  // Some parts of Brave's UI pages are not yet migrated to work without doing
  // assignments of strings directly into |innerHTML| elements (i.e. see usage
  // of |dangerouslySetInnerHTML| in .tsx files). This will break Brave due to
  // committing a Trusted Types related violation now that Trusted Types are
  // enforced on WebUI pages (see crrev.com/c/2234238 and crrev.com/c/2353547).
  // We should migrate those pages not to require using |innerHTML|, but for now
  // we just restore pre-Cromium 87 behaviour for pages that are not ready yet.
  if (disable_trusted_types_csp) {
    source->DisableTrustedTypesCSP();
  } else {
    // Allow a policy to be created so that we
    // can allow trusted HTML and trusted lazy-load script sources.
    source->OverrideContentSecurityPolicy(
        network::mojom::CSPDirectiveName::TrustedTypes,
        "default");
  }

  source->UseStringsJs();
  source->SetDefaultResource(html_resource_id);
  // Add generated resource paths
  for (size_t i = 0; i < resource_map_size; ++i) {
    source->AddResourcePath(resource_map[i].name,  resource_map[i].value);
  }
  CustomizeWebUIHTMLSource(name, source);
  return source;
}

BasicUI::BasicUI(content::WebUI* web_ui,
                 const std::string& name,
                 const GritResourceMap* resource_map,
                 size_t resource_map_size,
                 int html_resource_id,
                 bool disable_trusted_types_csp)
    : WebUIController(web_ui) {
  Profile* profile = Profile::FromWebUI(web_ui);
  content::WebUIDataSource* source = CreateBasicUIHTMLSource(profile, name,
      resource_map, resource_map_size, html_resource_id,
      disable_trusted_types_csp);
  content::WebUIDataSource::Add(profile, source);
}

BasicUI::~BasicUI() = default;
