// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/settings/brave_extensions_manifest_v2_installer.h"

#include <string>
#include <string_view>
#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/escape.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/constants/brave_services_key.h"
#include "brave/components/constants/brave_services_key_helper.h"
#include "brave/components/constants/network_constants.h"
#include "chrome/browser/extensions/crx_installer.h"
#include "chrome/browser/extensions/extension_install_prompt.h"
#include "chrome/browser/profiles/profile.h"
#include "components/crx_file/crx_verifier.h"
#include "components/update_client/update_query_params.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/crx_file_info.h"
#include "extensions/common/extension_urls.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "url/gurl.h"

namespace extensions_mv2 {

namespace {

bool IsKnownMV2Extension(std::string_view id) {
  return kPreconfiguredManifestV2Extensions.contains(id);
}

GURL GetUpdaterExtensionDownloadUrl(std::string_view extenion_id) {
  const std::string params[] = {base::JoinString({"id", extenion_id}, "="),
                                "installsource=ondemand", "uc"};
  const GURL url(
      extension_urls::GetWebstoreUpdateUrl().spec() + "?response=redirect&" +
      update_client::UpdateQueryParams::Get(
          update_client::UpdateQueryParams::CRX) +
      "&x=" + base::EscapeQueryParamValue(base::JoinString(params, "&"), true));
  CHECK(url.is_valid());
  return url;
}

GURL GetCrxDownloadUrl(const base::Value::Dict& update_manifest,
                       std::string_view extension_id) {
  const auto* gupdate = update_manifest.FindDict("gupdate");
  if (!gupdate) {
    return GURL();
  }
  const auto* app_list = gupdate->FindList("app");
  if (!app_list) {
    return GURL();
  }

  for (const auto& appv : *app_list) {
    if (!appv.is_dict()) {
      continue;
    }
    const auto& app = appv.GetDict();

    const auto* id = app.FindString("appid");
    if (!id || *id != extension_id) {
      continue;
    }

    const auto* update_check = app.FindDict("updatecheck");
    if (!update_check) {
      break;
    }
    const auto* codebase = update_check->FindString("codebase");
    if (!codebase) {
      break;
    }
    const GURL url(*codebase);
    if (!url.is_valid() || !brave::ShouldAddBraveServicesKeyHeader(url)) {
      break;
    }
    return url;
  }

  return GURL();
}

}  // namespace

ExtensionManifestV2Installer::ExtensionManifestV2Installer(
    const std::string& extension_id,
    content::WebContents* web_contents,
    extensions::WebstoreInstallWithPrompt::Callback callback)
    : extension_id_(extension_id),
      web_contents_(web_contents->GetWeakPtr()),
      callback_(std::move(callback)) {
  CHECK(IsKnownMV2Extension(extension_id));
  auto* profile =
      Profile::FromBrowserContext(web_contents_->GetBrowserContext());
  url_loader_factory_ = profile->GetURLLoaderFactory();
}

ExtensionManifestV2Installer::~ExtensionManifestV2Installer() = default;

void ExtensionManifestV2Installer::BeginInstall() {
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = GetUpdaterExtensionDownloadUrl(extension_id_);
  request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  request->headers.SetHeader("Content-Type", "application/json");
  request->headers.SetHeader(kBraveServicesKeyHeader,
                             BUILDFLAG(BRAVE_SERVICES_KEY));

  constexpr auto kExtensionsMV2ManifestAnnotationTag =
      net::DefineNetworkTrafficAnnotation("extensions_mv2_request", R"(
    semantics {
      sender: "Extension Manifest V2 Installer"
      description:
        "In response to this request Brave backend returns an JSON file
        with update/download response."
      trigger:
        "The user enables MV2 extension on the settings page"
      destination: BRAVE_OWNED_SERVICE
    }
    policy {
      cookies_allowed: NO
    })");

  url_loader_ = network::SimpleURLLoader::Create(
      std::move(request), kExtensionsMV2ManifestAnnotationTag);
  url_loader_->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory_.get(),
      base::BindOnce(&ExtensionManifestV2Installer::OnUpdateManifestResponse,
                     weak_factory_.GetWeakPtr()));
}

void ExtensionManifestV2Installer::OnUpdateManifestResponse(
    std::optional<std::string> body) {
  if (body) {
    auto update_manifest = base::JSONReader::ReadDict(body.value());
    if (update_manifest) {
      const GURL crx_url = GetCrxDownloadUrl(*update_manifest, extension_id_);
      if (crx_url.is_valid() && !crx_url.is_empty()) {
        return DownloadCrx(crx_url);
      }
    }
  }

  std::move(callback_).Run(false, "Failed to download extension.",
                           extensions::webstore_install::Result::OTHER_ERROR);
}

void ExtensionManifestV2Installer::DownloadCrx(const GURL& url) {
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = url;
  request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  request->headers.SetHeader(kBraveServicesKeyHeader,
                             BUILDFLAG(BRAVE_SERVICES_KEY));

  constexpr auto kExtensionsMV2CrxAnnotationTag =
      net::DefineNetworkTrafficAnnotation("extensions_mv2_request", R"(
    semantics {
      sender: "Extension Manifest V2 Installer"
      description:
        "In response to this request Brave backend returns crx file.
      trigger:
        "The user enables MV2 extension on the settings page"
      destination: BRAVE_OWNED_SERVICE
    }
    policy {
      cookies_allowed: NO
    })");

  url_loader_ = network::SimpleURLLoader::Create(
      std::move(request), kExtensionsMV2CrxAnnotationTag);
  url_loader_->DownloadToTempFile(
      url_loader_factory_.get(),
      base::BindOnce(&ExtensionManifestV2Installer::OnCrxDownloaded,
                     weak_factory_.GetWeakPtr()));
}

void ExtensionManifestV2Installer::OnCrxDownloaded(base::FilePath path) {
  if (path.empty()) {
    return std::move(callback_).Run(
        false, "Failed to download extension.",
        extensions::webstore_install::Result::OTHER_ERROR);
  }
  if (!web_contents_) {
    return std::move(callback_).Run(
        false, "Installation cancelled.",
        extensions::webstore_install::Result::USER_CANCELLED);
  }

  extensions::CRXFileInfo crx;
  crx.path = std::move(path);
  crx.required_format = crx_file::VerifierFormat::CRX3;

  crx_installer_ = extensions::CrxInstaller::Create(
      web_contents_->GetBrowserContext(),
      std::make_unique<ExtensionInstallPrompt>(web_contents_.get()));
  crx_installer_->set_expected_id(extension_id_);
  crx_installer_->set_is_gallery_install(true);
  crx_installer_->AddInstallerCallback(base::BindOnce(
      &ExtensionManifestV2Installer::OnInstalled, weak_factory_.GetWeakPtr()));
  crx_installer_->InstallCrxFile(crx);
}

void ExtensionManifestV2Installer::OnInstalled(
    const std::optional<extensions::CrxInstallError>& error) {
  if (!error) {
    return std::move(callback_).Run(true, std::string(),
                                    extensions::webstore_install::SUCCESS);
  }

  std::move(callback_).Run(false, base::UTF16ToUTF8(error->message()),
                           extensions::webstore_install::OTHER_ERROR);
}

}  // namespace extensions_mv2
