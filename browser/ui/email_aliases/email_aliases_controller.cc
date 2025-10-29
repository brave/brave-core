/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/email_aliases/email_aliases_controller.h"

#include "base/check_is_test.h"
#include "brave/browser/ui/webui/email_aliases/email_aliases_panel_ui.h"
#include "brave/components/email_aliases/features.h"
#include "chrome/browser/ui/singleton_tabs.h"
#include "chrome/browser/ui/views/bubble/webui_bubble_manager.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/context_menu_params.h"
#include "content/public/browser/web_contents.h"
#include "third_party/blink/public/mojom/forms/form_control_type.mojom-shared.h"
#include "url/gurl.h"

namespace {
constexpr char kEmailAliasesSettingsURL[] = "brave://settings/email-aliases";

static bool g_autoclose_bubble_for_testing = false;

}  // namespace

namespace email_aliases {

EmailAliasesController::EmailAliasesController(
    BrowserView* browser_view,
    EmailAliasesService* email_aliases_service)
    : browser_view_(browser_view),
      email_aliases_service_(email_aliases_service) {
  CHECK(base::FeatureList::IsEnabled(email_aliases::features::kEmailAliases));
  CHECK(browser_view_);
  CHECK(email_aliases_service_.get());
}

EmailAliasesController::~EmailAliasesController() = default;

bool EmailAliasesController::IsAvailableFor(
    const content::ContextMenuParams& params) const {
  if (!params.form_control_type) {
    return false;
  }

  using blink::mojom::FormControlType;
  return params.is_content_editable_for_autofill ||
         params.form_control_type == FormControlType::kInputEmail ||
         params.form_control_type == FormControlType::kInputText;
}

void EmailAliasesController::ShowBubble(uint64_t field_renderer_id) {
  if (!email_aliases_service_->IsReadyToCreate()) {
    return OpenSettingsPage();
  }

  CloseBubble();

  const auto url_with_field =
      base::StrCat({kEmailAliasesPanelURL,
                    "?field=", base::NumberToString(field_renderer_id)});

  auto* anchor_view = browser_view_->GetLocationBarView();

  bubble_ = WebUIBubbleManager::Create<EmailAliasesPanelUI>(
      anchor_view, browser_view_->browser(), GURL(url_with_field),
      IDS_SETTINGS_EMAIL_ALIASES_LABEL);
  if (g_autoclose_bubble_for_testing) {
    CHECK_IS_TEST();
    bubble_->DisableCloseBubbleHelperForTesting();  // IN-TEST
  }

  bubble_->ShowBubble(std::nullopt, views::BubbleBorder::TOP_CENTER);
  if (bubble_->GetBubbleWidget()) {
    bubble_->GetBubbleWidget()->SetVisible(true);
  }
}

void EmailAliasesController::CloseBubble() {
  bubble_.reset();
}

void EmailAliasesController::OpenSettingsPage() {
  CloseBubble();
  ShowSingletonTabOverwritingNTP(browser_view_->browser(),
                                 GURL(kEmailAliasesSettingsURL));
}

WebUIBubbleManager* EmailAliasesController::GetBubbleForTesting() {
  return bubble_.get();
}

void EmailAliasesController::OnAliasCreationComplete(const std::string& email) {
  CloseBubble();
  if (!email.empty() && browser_view_->GetActiveWebContents()) {
    // In future: consider to use AutofillDriver::ApplyFieldAction.
    browser_view_->GetActiveWebContents()->Replace(base::UTF8ToUTF16(email));
  }
}

// static
void EmailAliasesController::DisableAutoCloseBubbleForTesting(
    bool disale_autoclose) {
  g_autoclose_bubble_for_testing = disale_autoclose;
}

}  // namespace email_aliases
