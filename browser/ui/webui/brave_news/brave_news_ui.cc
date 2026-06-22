// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_news/brave_news_ui.h"

#include <string>

#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/memory/ref_counted_memory.h"
#include "brave/components/brave_news/common/features.h"
#include "brave/components/constants/webui_url_constants.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"

namespace {

// Placeholder page served until the Brave News frontend is wired up.
constexpr char kPlaceholderHtml[] =
    "<!doctype html><meta charset=\"utf-8\"><title>Brave News</title>"
    "<body>Brave News</body>";

}  // namespace

BraveNewsUI::BraveNewsUI(content::WebUI* web_ui)
    : UntrustedTopChromeWebUIController(web_ui) {
  auto* browser_context = web_ui->GetWebContents()->GetBrowserContext();
  content::WebUIDataSource* source =
      content::WebUIDataSource::CreateAndAdd(browser_context, kBraveNewsURL);

  // Serve the placeholder for every path; the real resources are added once
  // the frontend bundle is wired up.
  source->SetRequestFilter(
      base::BindRepeating([](const std::string& path) { return true; }),
      base::BindRepeating(
          [](const std::string& path,
             content::WebUIDataSource::GotDataCallback callback) {
            std::move(callback).Run(
                base::MakeRefCounted<base::RefCountedString>(
                    std::string(kPlaceholderHtml)));
          }));
}

BraveNewsUI::~BraveNewsUI() = default;

WEB_UI_CONTROLLER_TYPE_IMPL(BraveNewsUI)

BraveNewsUIConfig::BraveNewsUIConfig()
    : DefaultTopChromeWebUIConfig(content::kChromeUIUntrustedScheme,
                                  kBraveNewsHost) {}

bool BraveNewsUIConfig::IsWebUIEnabled(
    content::BrowserContext* browser_context) {
  return base::FeatureList::IsEnabled(brave_news::features::kBraveNewsSidebar);
}
