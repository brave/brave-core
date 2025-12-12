/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_wallet/trezor/trezor_ui.h"

#include <string>

#include "base/memory/ref_counted_memory.h"
#include "base/notreached.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/trezor_bridge/resources/grit/trezor_bridge_generated.h"
#include "brave/components/trezor_bridge/resources/grit/trezor_bridge_generated_map.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_data_source.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/webui/resources/grit/webui_resources.h"

namespace {
constexpr char kTrezorConnectURL[] = "https://connect.trezor.io/";

// Trezor popup page is opened with window.open(url) which doesn't
// work(opened window loses opener) from chrome-untrusted trezor bridge. We need
// to revert to legacy behavior which works: open blank page then navigate.
// So effectively we rollback this change:
// https://github.com/trezor/trezor-suite/pull/10975/changes#diff-38dd02260cff108b8329d6f3adbf8717b8e8737222de87a92c048f8bbf0bf159R256-R258
// by overwriting
// window.open(url, ...args)
// with
// window.open('', ...args).location.href = url
constexpr char kTrezorBundlePatch[] =
    "(()=>{"
    " window.open = new Proxy(window.open, {"
    "  apply(target, thisArg, [url, ...rest]) {"
    "   const result = Reflect.apply(target, thisArg, ['', ...rest]);"
    "   if (result && url) {"
    "    result.location.href = url;"
    "   }"
    "   return result;"
    "  }"
    " })"
    "})();\n";

const webui::ResourcePath* GetTrezorBundleResource() {
  for (auto& resource : kTrezorBridgeGenerated) {
    if (resource.id == IDR_TREZOR_BRIDGE_TREZOR_BUNDLE_JS) {
      return &resource;
    }
  }
  NOTREACHED();
}

bool ShouldHandleWebUIRequestCallback(const std::string& path) {
  return path == GetTrezorBundleResource()->path;
}

void HandleWebUIRequestCallback(
    const std::string& path,
    content::WebUIDataSource::GotDataCallback callback) {
  DCHECK(ShouldHandleWebUIRequestCallback(path));

  const auto trezor_bundle =
      ui::ResourceBundle::GetSharedInstance().LoadDataResourceString(
          GetTrezorBundleResource()->id);

  const auto patched_trezor_bundle = kTrezorBundlePatch + trezor_bundle;

  std::move(callback).Run(base::MakeRefCounted<base::RefCountedString>(
      std::move(patched_trezor_bundle)));
}

}  // namespace

namespace trezor {

UntrustedTrezorUI::UntrustedTrezorUI(content::WebUI* web_ui)
    : ui::UntrustedWebUIController(web_ui) {
  auto* untrusted_source = content::WebUIDataSource::CreateAndAdd(
      web_ui->GetWebContents()->GetBrowserContext(), kUntrustedTrezorURL);
  untrusted_source->SetDefaultResource(IDR_BRAVE_WALLET_TREZOR_BRIDGE_HTML);
  untrusted_source->AddFrameAncestor(GURL(kBraveUIWalletPageURL));
  untrusted_source->AddFrameAncestor(GURL(kBraveUIWalletPanelURL));
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ConnectSrc,
      "connect-src 'self' https://connect.trezor.io;");
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      std::string("script-src chrome://resources/js/ 'self' ") +
          kTrezorConnectURL + ";");
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameSrc,
      std::string("frame-src ") + kTrezorConnectURL + ";");
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::StyleSrc,
      std::string("style-src 'unsafe-inline';"));
  untrusted_source->AddResourcePath("load_time_data_deprecated.js",
                                    IDR_WEBUI_JS_LOAD_TIME_DATA_DEPRECATED_JS);
  untrusted_source->UseStringsJs();
  untrusted_source->AddString("braveWalletTrezorBridgeUrl",
                              kUntrustedTrezorURL);
  untrusted_source->SetRequestFilter(
      base::BindRepeating(&ShouldHandleWebUIRequestCallback),
      base::BindRepeating(HandleWebUIRequestCallback));
}

UntrustedTrezorUI::~UntrustedTrezorUI() = default;

std::unique_ptr<content::WebUIController>
UntrustedTrezorUIConfig::CreateWebUIController(content::WebUI* web_ui,
                                               const GURL& url) {
  return std::make_unique<UntrustedTrezorUI>(web_ui);
}

UntrustedTrezorUIConfig::UntrustedTrezorUIConfig()
    : WebUIConfig(content::kChromeUIUntrustedScheme, kUntrustedTrezorHost) {}

}  // namespace trezor
