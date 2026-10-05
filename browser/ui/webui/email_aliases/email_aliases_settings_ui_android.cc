/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/email_aliases/email_aliases_settings_ui_android.h"

#include <utility>

#include "base/check.h"
#include "base/check_deref.h"
#include "base/strings/strcat.h"
#include "brave/browser/brave_account/brave_account_dialog_mode_holder.h"
#include "brave/browser/brave_account/brave_account_service_factory.h"
#include "brave/browser/email_aliases/email_aliases_service_factory.h"
#include "brave/browser/ui/brave_account/brave_account_dialog_opener.h"
#include "brave/components/brave_account/brave_account_service.h"
#include "brave/components/brave_account/resources/grit/brave_account_resources_map.h"
#include "brave/components/email_aliases/constants.h"
#include "brave/components/email_aliases/email_aliases_service.h"
#include "brave/components/email_aliases/features.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_webui_strings.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"
#include "services/network/public/mojom/content_security_policy.mojom.h"
#include "ui/base/webui/resource_path.h"
#include "ui/webui/webui_util.h"

namespace {

constexpr webui::ResourcePath kEmailAliasesSettingsResources[] = {
    {"email_aliases.bundle.js", IDR_EMAIL_ALIASES_MANAGEMENT_BUNDLE_JS},
    {"email_aliases_settings.bundle.js", IDR_EMAIL_ALIASES_SETTINGS_BUNDLE_JS},
};

}  // namespace

EmailAliasesSettingsUIAndroid::EmailAliasesSettingsUIAndroid(
    content::WebUI* web_ui)
    : content::WebUIController(web_ui) {
  CHECK(email_aliases::features::IsEmailAliasesEnabled());

  auto* profile = Profile::FromWebUI(web_ui);
  auto* source = content::WebUIDataSource::CreateAndAdd(
      profile, email_aliases::kEmailAliasesSettingsHost);
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::StyleSrc,
      "style-src 'self' 'unsafe-inline' chrome://resources chrome://theme;");
  webui::SetupWebUIDataSource(source, kEmailAliasesSettingsResources,
                              IDR_EMAIL_ALIASES_SETTINGS_HTML);
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::TrustedTypes,
      base::StrCat(
          {webui::kDefaultTrustedTypesPolicies, " 'allow-duplicates';"}));
  source->AddResourcePaths(kBraveAccountResources);
  source->AddLocalizedStrings(webui::kBraveAccountStrings);
  source->AddLocalizedStrings(webui::kBraveAccountSharedStrings);
  source->AddLocalizedStrings(webui::kBraveAccountSettingsStrings);
  source->AddLocalizedStrings(webui::kEmailAliasesStrings);
}

EmailAliasesSettingsUIAndroid::~EmailAliasesSettingsUIAndroid() = default;

void EmailAliasesSettingsUIAndroid::BindInterface(
    mojo::PendingReceiver<email_aliases::mojom::EmailAliasesService> receiver) {
  email_aliases::EmailAliasesServiceFactory::BindForProfile(
      Profile::FromWebUI(web_ui()), std::move(receiver));
}

void EmailAliasesSettingsUIAndroid::BindInterface(
    mojo::PendingReceiver<email_aliases::mojom::EmailAliasesMetrics> receiver) {
  auto* service =
      email_aliases::EmailAliasesServiceFactory::GetServiceForProfile(
          Profile::FromWebUI(web_ui()));
  if (service) {
    service->metrics().BindInterface(std::move(receiver));
  }
}

void EmailAliasesSettingsUIAndroid::BindInterface(
    mojo::PendingReceiver<brave_account::mojom::Authentication> receiver) {
  auto* brave_account_service =
      brave_account::BraveAccountServiceFactory::GetFor(
          Profile::FromWebUI(web_ui()));
  CHECK_DEREF(brave_account_service).BindInterface(std::move(receiver));
}

void EmailAliasesSettingsUIAndroid::BindInterface(
    mojo::PendingReceiver<brave_account::mojom::DialogController> receiver) {
  dialog_controller_receiver_.reset();
  dialog_controller_receiver_.Bind(std::move(receiver));
}

void EmailAliasesSettingsUIAndroid::BindInterface(
    mojo::PendingReceiver<brave_account::mojom::DialogOpener>
        pending_receiver) {
  dialog_opener_receiver_.reset();
  dialog_opener_receiver_.Bind(std::move(pending_receiver));
}

void EmailAliasesSettingsUIAndroid::OpenDialog(
    const std::string& initiating_service_name,
    brave_account::mojom::DialogMode dialog_mode) {
  brave_account::OpenBraveAccountDialog(CHECK_DEREF(web_ui()->GetWebContents()),
                                        initiating_service_name, dialog_mode);
}

void EmailAliasesSettingsUIAndroid::CloseDialog() {
  web_ui()->GetWebContents()->Close();
}

void EmailAliasesSettingsUIAndroid::GetDialogMode(
    GetDialogModeCallback callback) {
  std::move(callback).Run(
      brave_account::BraveAccountDialogModeHolder::GetDialogMode(
          CHECK_DEREF(web_ui()->GetWebContents())));
}

WEB_UI_CONTROLLER_TYPE_IMPL(EmailAliasesSettingsUIAndroid)

EmailAliasesSettingsUIAndroidConfig::EmailAliasesSettingsUIAndroidConfig()
    : content::DefaultWebUIConfig<EmailAliasesSettingsUIAndroid>(
          content::kChromeUIScheme,
          email_aliases::kEmailAliasesSettingsHost) {}
