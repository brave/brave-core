/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_webui_source.h"

#include <string_view>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/containers/span.h"
#include "base/strings/utf_string_conversions.h"
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
#include "brave/browser/ui/webui/navigation_bar_data_provider.h"
#include "brave/components/brave_rewards/resources/grit/brave_rewards_resources.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/grit/branded_strings.h"
#include "content/public/browser/web_contents.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/color/color_provider.h"
#include "ui/color/color_provider_utils.h"
#endif

namespace {

void CustomizeWebUIHTMLSource(content::WebUI* web_ui,
                              const std::string& name,
                              content::WebUIDataSource* source) {
#if !BUILDFLAG(IS_ANDROID)
  if (name == "rewards" || name == "wallet") {
    NavigationBarDataProvider::Initialize(source, Profile::FromWebUI(web_ui));
  }
#endif

  source->AddResourcePaths(brave::GetWebUIResources(name));
  source->AddLocalizedStrings(brave::GetWebUILocalizedStrings(name));
}

content::WebUIDataSource* CreateWebUIDataSource(
    content::WebUI* web_ui,
    const std::string& name,
    base::span<const webui::ResourcePath> resource_map,
    size_t resource_map_size,
    int html_resource_id,
    bool disable_trusted_types_csp) {
  content::WebUIDataSource* source =
      content::WebUIDataSource::CreateAndAdd(Profile::FromWebUI(web_ui), name);
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
        "trusted-types default;");
  }

  source->UseStringsJs();
  source->SetDefaultResource(html_resource_id);
  // Add generated resource paths
  for (const auto& resource : resource_map) {
    source->AddResourcePath(resource.path, resource.id);
  }
  CustomizeWebUIHTMLSource(web_ui, name, source);
  return source;
}

}  // namespace

content::WebUIDataSource* CreateAndAddWebUIDataSource(
    content::WebUI* web_ui,
    const std::string& name,
    base::span<const webui::ResourcePath> resource_map,
    size_t resource_map_size,
    int html_resource_id,
    bool disable_trusted_types_csp) {
  content::WebUIDataSource* data_source =
      CreateWebUIDataSource(web_ui, name, resource_map, resource_map_size,
                            html_resource_id, disable_trusted_types_csp);
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
    const Browser* browser = chrome::FindBrowserWithProfile(profile);
    if (browser) {
      browser_window = browser->window();
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
