// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/email_aliases/email_aliases_bubble_ui.h"

#include <memory>
#include <string>
#include <utility>

#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/browser/ui/webui/settings/brave_email_aliases_handler.h"
#include "brave/browser/ui/webui/settings/brave_settings_localized_strings_provider.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/email_aliases/browser/resources/grit/email_aliases_bubble_generated_map.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/views/bubble/webui_bubble_manager.h"
#include "chrome/browser/ui/webui/top_chrome/top_chrome_webui_config.h"
#include "components/autofill/content/browser/content_autofill_driver.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/common/url_constants.h"
#include "ui/base/mojom/dialog_button.mojom.h"
#include "ui/views/view.h"
#include "url/gurl.h"

namespace email_aliases {

// static
std::unique_ptr<WebUIBubbleManager> EmailAliasesBubbleUI::webui_bubble_manager_;
raw_ptr<content::WebContents> EmailAliasesBubbleUI::web_contents_ = nullptr;
uint64_t EmailAliasesBubbleUI::field_renderer_id_ = 0;

// static
void EmailAliasesBubbleUI::Show(
    BrowserWindowInterface* browser_window_interface,
    views::View* anchor_view,
    content::WebContents* web_contents,
    uint64_t field_renderer_id) {
  EmailAliasesBubbleUI::Close();
  EmailAliasesBubbleUI::field_renderer_id_ = field_renderer_id;
  EmailAliasesBubbleUI::web_contents_ = web_contents;
  webui_bubble_manager_ = WebUIBubbleManager::Create<EmailAliasesBubbleUI>(
      anchor_view, browser_window_interface, GURL(kEmailAliasesBubbleURL),
      IDS_BRAVE_SHIELDS);
  webui_bubble_manager_->ShowBubble(std::nullopt,
                                    views::BubbleBorder::TOP_CENTER);
  webui_bubble_manager_->GetBubbleWidget()->SetVisible(true);
}

// static
void EmailAliasesBubbleUI::Close() {
  if (webui_bubble_manager_ && webui_bubble_manager_->GetBubbleWidget()) {
    webui_bubble_manager_->CloseBubble();
    webui_bubble_manager_.reset(nullptr);
  }
}

// static
void EmailAliasesBubbleUI::FillField(const std::string& alias_address) {
  if (!EmailAliasesBubbleUI::web_contents_) {
    return;
  }
  content::RenderFrameHost* render_frame_host =
      EmailAliasesBubbleUI::web_contents_->GetPrimaryMainFrame();
  if (!render_frame_host) {
    return;
  }
  autofill::AutofillDriver* driver =
      autofill::ContentAutofillDriver::GetForRenderFrameHost(render_frame_host);
  if (!driver) {
    return;
  }
  autofill::LocalFrameToken frame_token = driver->GetFrameToken();
  auto field_global_id = autofill::FieldGlobalId(
      frame_token,
      autofill::FieldRendererId(EmailAliasesBubbleUI::field_renderer_id_));
  driver->ApplyFieldAction(autofill::mojom::FieldActionType::kReplaceAll,
                           autofill::mojom::ActionPersistence::kFill,
                           field_global_id, base::UTF8ToUTF16(alias_address));
}

EmailAliasesBubbleUI::EmailAliasesBubbleUI(content::WebUI* web_ui)
    : TopChromeWebUIController(web_ui, true) {
  auto* source = CreateAndAddWebUIDataSource(web_ui, kEmailAliasesBubbleHost,
                                             kEmailAliasesBubbleGenerated,
                                             IDR_EMAIL_ALIASES_BUBBLE_HTML);
  web_ui->AddMessageHandler(std::make_unique<BraveEmailAliasesHandler>());
  settings::BraveAddEmailAliasesStrings(source);
  DCHECK(source);
}

EmailAliasesBubbleUI::~EmailAliasesBubbleUI() = default;

bool EmailAliasesBubbleUIConfig::IsWebUIEnabled(
    content::BrowserContext* browser_context) {
  return true;
}

bool EmailAliasesBubbleUIConfig::ShouldAutoResizeHost() {
  return true;
}

WEB_UI_CONTROLLER_TYPE_IMPL(EmailAliasesBubbleUI)

}  // namespace email_aliases
