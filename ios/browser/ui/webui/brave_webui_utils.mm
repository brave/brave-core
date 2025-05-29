// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ui/webui/brave_webui_utils.h"

#include <string_view>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/constants/url_constants.h"
#include "brave/components/webui/webui_resources.h"
#include "brave/ios/browser/ui/webui/brave_web_ui_ios_data_source.h"
#include "build/build_config.h"
#include "components/grit/components_resources.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/webui/web_ui_ios_data_source.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/webui/resource_path.h"
#include "ui/base/webui/web_ui_util.h"

namespace {

void CustomizeWebUIHTMLSource(web::WebUIIOS* web_ui,
                              const std::string& name,
                              web::WebUIIOSDataSource* source) {
  source->AddResourcePaths(brave::GetWebUIResources(name));
  source->AddLocalizedStrings(brave::GetWebUILocalizedStrings(name));
}

}  // namespace

namespace brave {

BraveWebUIIOSDataSource* CreateAndAddWebUIDataSource(
    web::WebUIIOS* web_ui,
    const std::string& name,
    base::span<const webui::ResourcePath> resource_paths,
    int html_resource_id,
    bool disable_trusted_types_csp) {
  auto* source = BraveWebUIIOSDataSource::CreateAndAdd(
      ProfileIOS::FromWebUIIOS(web_ui), name);

  source->AddResourcePaths(resource_paths);
  source->SetDefaultResource(html_resource_id);

  source->UseStringsJs();
  source->EnableReplaceI18nInJS();

  CustomizeWebUIHTMLSource(web_ui, name, source);

  if (disable_trusted_types_csp) {
    source->DisableTrustedTypesCSP();
  }

  return source;
}

}  // namespace brave
