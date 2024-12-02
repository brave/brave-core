/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/component_updater/brave_component_updater_configurator.h"

#include <stdint.h>

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/sys_string_conversions.h"
#include "base/version.h"
#include "brave/components/constants/brave_switches.h"
#include "build/build_config.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/pref_names.h"
#include "components/component_updater/component_updater_command_line_config_policy.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/services/patch/content/patch_service.h"
#include "components/services/unzip/content/unzip_service.h"
#include "components/update_client/activity_data_service.h"
#include "components/update_client/crx_downloader_factory.h"
#include "components/update_client/net/network_chromium.h"
#include "components/update_client/patch/patch_impl.h"
#include "components/update_client/persisted_data.h"
#include "components/update_client/protocol_handler.h"
#include "components/update_client/unzip/unzip_impl.h"
#include "components/update_client/unzipper.h"
#include "components/update_client/update_query_params.h"
#include "content/public/browser/browser_thread.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

#if BUILDFLAG(IS_WIN)
#include "base/win/win_util.h"
#include "chrome/installer/util/google_update_settings.h"
#endif

namespace component_updater {

// Allows the component updater to use non-encrypted communication with the
// update backend. The security of the update checks is enforced using
// a custom message signing protocol and it does not depend on using HTTPS.
BraveConfigurator::BraveConfigurator(
    const base::CommandLine* cmdline,
    PrefService* pref_service,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : configurator_impl_(ComponentUpdaterCommandLineConfigPolicy(cmdline),
                         false),
      pref_service_(raw_ref<PrefService>::from_ptr(pref_service)),
      persisted_data_(update_client::CreatePersistedData(
          base::BindRepeating(
              [](PrefService* pref_service) { return pref_service; },
              pref_service),
          nullptr)),
      url_loader_factory_(std::move(url_loader_factory)) {}

BraveConfigurator::~BraveConfigurator() = default;

base::TimeDelta BraveConfigurator::InitialDelay() const {
  return configurator_impl_.InitialDelay();
}

base::TimeDelta BraveConfigurator::NextCheckDelay() const {
  auto* command = base::CommandLine::ForCurrentProcess();
  if (command->HasSwitch(switches::kComponentUpdateIntervalInSec)) {
    int interval = 0;
    if (base::StringToInt(command->GetSwitchValueASCII(
                              switches::kComponentUpdateIntervalInSec),
                          &interval)) {
      DCHECK_GE(interval, 1);
      return base::Seconds(interval);
    }
  }
  return configurator_impl_.NextCheckDelay();
}

base::TimeDelta BraveConfigurator::OnDemandDelay() const {
  return configurator_impl_.OnDemandDelay();
}

base::TimeDelta BraveConfigurator::UpdateDelay() const {
  return configurator_impl_.UpdateDelay();
}

std::vector<GURL> BraveConfigurator::UpdateUrl() const {
  return configurator_impl_.UpdateUrl();
}

std::vector<GURL> BraveConfigurator::PingUrl() const {
  return configurator_impl_.PingUrl();
}

std::string BraveConfigurator::GetProdId() const {
  return std::string();
}

base::Version BraveConfigurator::GetBrowserVersion() const {
  return configurator_impl_.GetBrowserVersion();
}

std::string BraveConfigurator::GetChannel() const {
  return std::string("stable");
}

std::string BraveConfigurator::GetLang() const {
  return std::string();
}

std::string BraveConfigurator::GetOSLongName() const {
  return configurator_impl_.GetOSLongName();
}

base::flat_map<std::string, std::string>
BraveConfigurator::ExtraRequestParams() const {
  return configurator_impl_.ExtraRequestParams();
}

std::string BraveConfigurator::GetDownloadPreference() const {
  return std::string();
}

scoped_refptr<update_client::NetworkFetcherFactory>
BraveConfigurator::GetNetworkFetcherFactory() {
  if (!network_fetcher_factory_) {
    network_fetcher_factory_ =
        base::MakeRefCounted<update_client::NetworkFetcherChromiumFactory>(
            url_loader_factory_,
            // Never send cookies for component update downloads.
            base::BindRepeating([](const GURL& url) { return false; }));
  }
  return network_fetcher_factory_;
}

scoped_refptr<update_client::CrxDownloaderFactory>
BraveConfigurator::GetCrxDownloaderFactory() {
  if (!crx_downloader_factory_) {
    crx_downloader_factory_ =
        update_client::MakeCrxDownloaderFactory(GetNetworkFetcherFactory());
  }
  return crx_downloader_factory_;
}

scoped_refptr<update_client::UnzipperFactory>
BraveConfigurator::GetUnzipperFactory() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!unzip_factory_) {
    unzip_factory_ = base::MakeRefCounted<update_client::UnzipChromiumFactory>(
        base::BindRepeating(&unzip::LaunchUnzipper));
  }
  return unzip_factory_;
}

scoped_refptr<update_client::PatcherFactory>
BraveConfigurator::GetPatcherFactory() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!patch_factory_) {
    patch_factory_ = base::MakeRefCounted<update_client::PatchChromiumFactory>(
        base::BindRepeating(&patch::LaunchFilePatcher));
  }
  return patch_factory_;
}

bool BraveConfigurator::EnabledBackgroundDownloader() const {
  return configurator_impl_.EnabledBackgroundDownloader();
}

bool BraveConfigurator::EnabledCupSigning() const {
  return configurator_impl_.EnabledCupSigning();
}

PrefService* BraveConfigurator::GetPrefService() const {
  return base::to_address(pref_service_);
}

update_client::PersistedData* BraveConfigurator::GetPersistedData() const {
  return persisted_data_.get();
}

bool BraveConfigurator::IsPerUserInstall() const {
  return false;
}

std::unique_ptr<update_client::ProtocolHandlerFactory>
BraveConfigurator::GetProtocolHandlerFactory() const {
  return configurator_impl_.GetProtocolHandlerFactory();
}

std::optional<bool> BraveConfigurator::IsMachineExternallyManaged() const {
  return std::nullopt;
}

update_client::UpdaterStateProvider BraveConfigurator::GetUpdaterStateProvider()
    const {
  // TODO(crbug.com/1286378) - add a dependency on //chrome/updater and
  // implement this function so that it picks up that updater state, in
  // addition to Omaha or Keystone updater states.
  return configurator_impl_.GetUpdaterStateProvider();
}

std::optional<base::FilePath> BraveConfigurator::GetCrxCachePath() const {
  base::FilePath path;
  bool result = base::PathService::Get(chrome::DIR_USER_DATA, &path);
  return result ? std::optional<base::FilePath>(
                      path.AppendASCII("component_crx_cache"))
                : std::nullopt;
}

bool BraveConfigurator::IsConnectionMetered() const {
  return configurator_impl_.IsConnectionMetered();
}

}  // namespace component_updater
