// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_new_tab_page_refresh/custom_widget_untrusted_ui.h"

#include <string>
#include <string_view>
#include <utility>

#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/memory/ref_counted_memory.h"
#include "base/strings/strcat.h"
#include "brave/browser/ui/brave_ui_features.h"
#include "brave/components/constants/webui_url_constants.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"
#include "services/network/public/mojom/content_security_policy.mojom.h"
#include "url/gurl.h"

namespace brave_new_tab_page_refresh {

namespace {

// Public, no-auth, CORS-friendly endpoints that custom widgets are allowed to
// fetch from. This list is intentionally small for the PoC; it must be kept in
// sync with the prompt contract shown to the user in the settings UI.
constexpr char kConnectSrc[] =
    "connect-src "
    "https://api.open-meteo.com "
    "https://geocoding-api.open-meteo.com "
    "https://api.coingecko.com "
    "https://api.frankfurter.app "
    "https://hacker-news.firebaseio.com;";

// Minimal host document. It only loads host.js; the actual (untrusted) widget
// markup is injected at runtime into a sandboxed child iframe.
constexpr char kHostHtml[] = R"(<!doctype html>
<html>
<head>
<meta charset="utf-8">
<style>
  html, body { margin: 0; padding: 0; height: 100%; background: transparent; }
</style>
</head>
<body>
<script src="host.js"></script>
</body>
</html>)";

// Receives a self-contained HTML widget document from the embedding New Tab
// Page and renders it in a sandboxed (opaque-origin) child iframe. Widget
// scripts therefore cannot script this page, cannot reach any browser bindings,
// and can only talk to the connect-src allow-list defined by this page's CSP.
constexpr char kHostJs[] = R"((function () {
  'use strict';

  // Only accept widget content from the trusted New Tab Page embedder.
  var kEmbedderOrigin = 'chrome://newtab';
  var frame = null;

  function render(html) {
    if (frame && frame.parentNode) {
      frame.parentNode.removeChild(frame);
    }
    frame = document.createElement('iframe');
    frame.setAttribute('sandbox', 'allow-scripts');
    frame.setAttribute('scrolling', 'no');
    frame.style.cssText =
      'border:0;width:100%;height:100%;display:block;' +
      'background:transparent;color-scheme:dark;';
    frame.srcdoc = typeof html === 'string' ? html : '';
    document.body.appendChild(frame);
  }

  window.addEventListener('message', function (event) {
    if (event.origin !== kEmbedderOrigin) {
      return;
    }
    var data = event.data;
    if (!data || data.type !== 'brave-ntp-widget') {
      return;
    }
    render(data.html);
  });

  if (window.parent !== window) {
    window.parent.postMessage({ type: 'brave-ntp-widget-ready' },
                              kEmbedderOrigin);
  }
})();)";

bool ShouldHandleRequest(const std::string& path) {
  return path.empty() || path == "host.html" || path == "host.js";
}

void HandleRequest(const std::string& path,
                   content::WebUIDataSource::GotDataCallback callback) {
  std::string_view body = (path == "host.js") ? kHostJs : kHostHtml;
  std::move(callback).Run(
      base::MakeRefCounted<base::RefCountedString>(std::string(body)));
}

}  // namespace

CustomWidgetUntrustedUIConfig::CustomWidgetUntrustedUIConfig()
    : WebUIConfig(content::kChromeUIUntrustedScheme, kBraveNTPWidgetsUIHost) {}

CustomWidgetUntrustedUIConfig::~CustomWidgetUntrustedUIConfig() = default;

bool CustomWidgetUntrustedUIConfig::IsWebUIEnabled(
    content::BrowserContext* browser_context) {
  return base::FeatureList::IsEnabled(features::kBraveNtpCustomWidgets);
}

std::unique_ptr<content::WebUIController>
CustomWidgetUntrustedUIConfig::CreateWebUIController(content::WebUI* web_ui,
                                                    const GURL& url) {
  return std::make_unique<CustomWidgetUntrustedUI>(web_ui);
}

CustomWidgetUntrustedUI::CustomWidgetUntrustedUI(content::WebUI* web_ui)
    : ui::UntrustedWebUIController(web_ui) {
  auto* browser_context = web_ui->GetWebContents()->GetBrowserContext();
  auto* source = content::WebUIDataSource::CreateAndAdd(browser_context,
                                                        kBraveNTPWidgetsUIURL);

  // The host script assigns the widget markup to a sandboxed iframe's srcdoc,
  // which is a Trusted Types sink. This page has no bindings and is isolated, so
  // it is safe to opt out of Trusted Types here.
  source->DisableTrustedTypesCSP();

  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::DefaultSrc, "default-src 'none';");
  // 'unsafe-inline' lets the injected widget run its own inline scripts; this
  // page has no bindings, so that capability is confined to the sandbox.
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      "script-src 'self' 'unsafe-inline';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::StyleSrc,
      "style-src 'unsafe-inline' https:;");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ImgSrc, "img-src https: data: blob:;");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FontSrc, "font-src https: data:;");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ConnectSrc, kConnectSrc);
  // Allow the sandboxed srcdoc child that actually hosts the widget.
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameSrc, "frame-src 'self';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ChildSrc, "child-src 'self';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ObjectSrc, "object-src 'none';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::BaseURI, "base-uri 'none';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FormAction, "form-action 'none';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::WorkerSrc, "worker-src 'none';");
  // Only the New Tab Page may embed this untrusted host.
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameAncestors,
      base::StrCat({"frame-ancestors ", kBraveUINewTabURL, ";"}));

  source->SetRequestFilter(base::BindRepeating(&ShouldHandleRequest),
                           base::BindRepeating(&HandleRequest));
}

CustomWidgetUntrustedUI::~CustomWidgetUntrustedUI() = default;

}  // namespace brave_new_tab_page_refresh
