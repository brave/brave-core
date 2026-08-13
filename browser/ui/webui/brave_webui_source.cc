/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_webui_source.h"

#include <string_view>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/containers/span.h"
#include "base/logging.h"
#include "brave/components/constants/url_constants.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "brave/components/webui/webui_resources.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "components/grit/components_resources.h"
#include "content/public/browser/web_ui_data_source.h"
#include "services/network/public/mojom/content_security_policy.mojom.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/webui/resource_path.h"
#include "ui/base/webui/web_ui_util.h"

#if !BUILDFLAG(IS_ANDROID)
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/browser_window/public/profile_browser_collection.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/grit/branded_strings.h"
#include "content/public/browser/web_contents.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/color/color_provider.h"
#include "ui/color/color_provider_utils.h"
#endif

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

// Android doesn't need WebUI WebContents to match background color
#if !BUILDFLAG(IS_ANDROID)

void AddBackgroundColorToSource(content::WebUIDataSource* source,
                                content::WebContents* contents) {
  // Get the specific background color for the type of browser window
  // that the contents is in.
  // TODO(petemill): we do not use web_contents->GetColorProvider()
  // here because it does not include BravePrivateWindowThemeSupplier. This
  // should get fixed, potentially via `WebContents::SetColorProviderSource`.
  auto* browser_window =
      BrowserWindow::FindBrowserWindowWithWebContents(contents);
  if (!browser_window) {
    // Some newly created WebContents aren't yet attached
    // to a browser window, so get any that match the current profile,
    // which is fine for color provider.
    Profile* profile =
        Profile::FromBrowserContext(contents->GetBrowserContext());
    if (auto* browser = ProfileBrowserCollection::GetForProfile(profile)
                            ->GetLastActiveBrowser()) {
      browser_window = BrowserWindow::FromBrowser(browser);
    }
  }
  if (!browser_window) {
    DLOG(ERROR) << "No BrowserWindow could be found for WebContents";
    return;
  }
  const ui::ColorProvider* color_provider = browser_window->GetColorProvider();
  SkColor ntp_background_color =
      color_provider->GetColor(kColorNewTabPageBackground);
  // Set to a template replacement string that can be inserted to the
  // html.
  std::string ntp_background_color_css =
      ui::ConvertSkColorToCSSColor(ntp_background_color);
  source->AddString("backgroundColor", ntp_background_color_css);
}

#endif  // !BUILDFLAG(IS_ANDROID)
