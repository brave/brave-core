/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_UI_BASE_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_UI_BASE_H_

#include <utility>

#include "base/check.h"
#include "base/check_deref.h"
#include "base/containers/span.h"
#include "base/functional/callback_forward.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/raw_ref.h"
#include "brave/components/brave_account/brave_account_service.h"
#include "brave/components/brave_account/features.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "brave/components/brave_account/resources/grit/brave_account_resources.h"
#include "brave/components/brave_account/resources/grit/brave_account_resources_map.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/password_strength_meter/password_strength_meter.h"
#include "brave/components/password_strength_meter/password_strength_meter.mojom.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "components/grit/brave_components_webui_strings.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "services/network/public/mojom/content_security_policy.mojom.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/webui/resource_path.h"

// Template base class for Brave Account WebUI controllers.
//
// `BraveAccountUIBase` encapsulates shared setup logic for Brave Account WebUIs
// across desktop, Android, and iOS. It streamlines the creation and
// configuration of a WebUIDataSource.
//
// Intended to be subclassed with the appropriate WebUIDataSource and
// BraveAccountServiceFactory types.
template <typename WebUIDataSource, typename BraveAccountServiceFactory>
class BraveAccountUIBase {
 public:
  template <typename Profile>
  explicit BraveAccountUIBase(
      Profile* profile,
      base::OnceCallback<void(WebUIDataSource*,
                              base::span<const webui::ResourcePath>,
                              int)> setup_webui_data_source = base::DoNothing())
      : brave_account_service_(
            CHECK_DEREF(BraveAccountServiceFactory::GetFor(profile))) {
    CHECK(brave_account::features::IsBraveAccountEnabled());

    auto* source = WebUIDataSource::CreateAndAdd(profile, kBraveAccountHost);
    std::move(setup_webui_data_source)
        .Run(source, kBraveAccountResources,
             IDR_BRAVE_ACCOUNT_BRAVE_ACCOUNT_PAGE_HTML);
    SetupWebUIDataSource(source);
  }

  void BindInterface(mojo::PendingReceiver<brave_account::mojom::Authentication>
                         pending_receiver) {
    brave_account_service_->BindInterface(std::move(pending_receiver));
  }

  void BindInterface(mojo::PendingReceiver<
                     password_strength_meter::mojom::PasswordStrengthMeter>
                         pending_receiver) {
    password_strength_meter::BindInterface(std::move(pending_receiver));
  }

 private:
  static inline constexpr char16_t kBraveAccountSelfCustodyLearnMoreURL[] =
      u"https://search.brave.com";
  static inline constexpr char16_t kBraveAccountTermsOfServiceURL[] =
      u"https://brave.com/terms-of-use/";
  static inline constexpr char16_t kBraveAccountPrivacyAgreementURL[] =
      u"https://brave.com/privacy/browser/";

  void SetupWebUIDataSource(WebUIDataSource* source) {
    source->OverrideContentSecurityPolicy(
        network::mojom::CSPDirectiveName::ScriptSrc,
        "script-src chrome://resources 'self' 'wasm-unsafe-eval';");
    source->OverrideContentSecurityPolicy(
        network::mojom::CSPDirectiveName::RequireTrustedTypesFor,
        "require-trusted-types-for 'script';");
    source->OverrideContentSecurityPolicy(
        network::mojom::CSPDirectiveName::TrustedTypes,
        "trusted-types lit-html-desktop;");

    source->UseStringsJs();
    source->EnableReplaceI18nInJS();

    source->AddResourcePaths(kBraveAccountResources);
    source->AddResourcePath("", IDR_BRAVE_ACCOUNT_BRAVE_ACCOUNT_PAGE_HTML);

    source->AddLocalizedStrings(webui::kBraveAccountStrings);
    source->AddString(
        "BRAVE_ACCOUNT_SELF_CUSTODY_DESCRIPTION",
        l10n_util::GetStringFUTF16(IDS_BRAVE_ACCOUNT_SELF_CUSTODY_DESCRIPTION,
                                   kBraveAccountSelfCustodyLearnMoreURL));
    source->AddString(
        "BRAVE_ACCOUNT_CONSENT_CHECKBOX_LABEL",
        l10n_util::GetStringFUTF16(IDS_BRAVE_ACCOUNT_CONSENT_CHECKBOX_LABEL,
                                   kBraveAccountTermsOfServiceURL,
                                   kBraveAccountPrivacyAgreementURL));

    source->AddResourcePath("full_brave_brand.svg",
                            IDR_BRAVE_ACCOUNT_IMAGES_FULL_BRAVE_BRAND_SVG);
    source->AddResourcePath("full_brave_brand_dark.svg",
                            IDR_BRAVE_ACCOUNT_IMAGES_FULL_BRAVE_BRAND_DARK_SVG);
  }

 private:
  const raw_ref<brave_account::BraveAccountService> brave_account_service_;
};

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_UI_BASE_H_
