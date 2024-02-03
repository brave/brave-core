// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/ai_chat/ai_chat_ui.h"

#include <utility>

#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/ui/side_panel/ai_chat/ai_chat_side_panel_utils.h"
#include "brave/browser/ui/webui/ai_chat/ai_chat_ui_page_handler.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/ai_chat/resources/page/grit/ai_chat_ui_generated_map.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/l10n/common/localization_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/webui_util.h"
#include "components/grit/brave_components_resources.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"

#if !BUILDFLAG(IS_ANDROID)
#include "chrome/browser/ui/browser.h"
#else
#include "chrome/browser/ui/android/tab_model/tab_model.h"
#include "chrome/browser/ui/android/tab_model/tab_model_list.h"
#endif

#if BUILDFLAG(IS_ANDROID)
namespace {
content::WebContents* GetActiveWebContents(content::BrowserContext* context) {
  auto tab_models = TabModelList::models();
  auto iter = base::ranges::find_if(
      tab_models, [](const auto& model) { return model->IsActiveModel(); });
  if (iter == tab_models.end()) {
    return nullptr;
  }

  auto* active_contents = (*iter)->GetActiveWebContents();
  if (!active_contents) {
    return nullptr;
  }
  DCHECK_EQ(active_contents->GetBrowserContext(), context);
  return active_contents;
}
}  // namespace
#endif

AIChatUI::AIChatUI(content::WebUI* web_ui)
    : ui::UntrustedWebUIController(web_ui),
      profile_(Profile::FromWebUI(web_ui)) {
  DCHECK(profile_);
  DCHECK(brave::IsRegularProfile(profile_));

  // Create a URLDataSource and add resources.
  content::WebUIDataSource* untrusted_source =
      content::WebUIDataSource::CreateAndAdd(
          web_ui->GetWebContents()->GetBrowserContext(), kChatUIURL);

  webui::SetupWebUIDataSource(
      untrusted_source,
      base::make_span(kAiChatUiGenerated, kAiChatUiGeneratedSize),
      IDR_CHAT_UI_HTML);

  untrusted_source->AddResourcePath("styles.css", IDR_CHAT_UI_CSS);

  for (const auto& str : ai_chat::GetLocalizedStrings()) {
    untrusted_source->AddString(
        str.name, brave_l10n::GetLocalizedResourceUTF16String(str.id));
  }

  base::Time last_accepted_disclaimer =
      profile_->GetOriginalProfile()->GetPrefs()->GetTime(
          ai_chat::prefs::kLastAcceptedDisclaimer);

  untrusted_source->AddBoolean("hasAcceptedAgreement",
                               !last_accepted_disclaimer.is_null());

  untrusted_source->AddBoolean(
      "hasUserDismissedPremiumPrompt",
      profile_->GetOriginalProfile()->GetPrefs()->GetBoolean(
          ai_chat::prefs::kUserDismissedPremiumPrompt));

  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      "script-src 'self' chrome-untrusted://resources;");
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::StyleSrc,
      "style-src 'self' 'unsafe-inline' chrome-untrusted://resources;");
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ImgSrc,
      "img-src 'self' blob: chrome-untrusted://resources;");
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FontSrc,
      "font-src 'self' data: chrome-untrusted://resources;");

  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::TrustedTypes, "trusted-types default;");
}

AIChatUI::~AIChatUI() = default;

void AIChatUI::BindInterface(
    mojo::PendingReceiver<ai_chat::mojom::PageHandler> receiver) {
  // We call ShowUI() before creating the PageHandler object so that
  // the WebContents is added to a Browser which we can get a reference
  // to and provide to the PageHandler.
  if (embedder_) {
    embedder_->ShowUI();
  }

  content::WebContents* web_contents = nullptr;
#if !BUILDFLAG(IS_ANDROID)
  raw_ptr<Browser> browser =
      ai_chat::GetBrowserForWebContents(web_ui()->GetWebContents());
  DCHECK(browser);
  TabStripModel* tab_strip_model = browser->tab_strip_model();
  DCHECK(tab_strip_model);
  web_contents = tab_strip_model->GetActiveWebContents();
#else
  web_contents = GetActiveWebContents(profile_);
#endif
  if (web_contents == web_ui()->GetWebContents()) {
    web_contents = nullptr;
  }
  page_handler_ = std::make_unique<ai_chat::AIChatUIPageHandler>(
      web_ui()->GetWebContents(), web_contents, profile_, std::move(receiver));
}

std::unique_ptr<content::WebUIController>
UntrustedChatUIConfig::CreateWebUIController(content::WebUI* web_ui,
                                             const GURL& url) {
  return std::make_unique<AIChatUI>(web_ui);
}

bool UntrustedChatUIConfig::IsWebUIEnabled(
    content::BrowserContext* browser_context) {
  return ai_chat::features::IsAIChatEnabled() &&
         brave::IsRegularProfile(browser_context);
}

UntrustedChatUIConfig::UntrustedChatUIConfig()
    : WebUIConfig(content::kChromeUIUntrustedScheme, kChatUIHost) {}

WEB_UI_CONTROLLER_TYPE_IMPL(AIChatUI)
