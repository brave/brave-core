/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/component_updater/brave_component_updater_configurator.h"

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "base/strings/sys_string_conversions.h"
#include "base/version.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/common/pref_names.h"
#include "components/component_updater/component_updater_command_line_config_policy.h"
#include "components/component_updater/configurator_impl.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/update_client/activity_data_service.h"
#include "components/update_client/net/network_chromium.h"
#include "components/update_client/protocol_handler.h"
#include "components/update_client/update_query_params.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/service_manager_connection.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/service_manager/public/cpp/connector.h"

#if defined(OS_WIN)
#include "base/win/win_util.h"
#include "chrome/installer/util/google_update_settings.h"
#endif

namespace component_updater {

namespace {

class BraveConfigurator : public update_client::Configurator {
 public:
  BraveConfigurator(const base::CommandLine* cmdline,
                    PrefService* pref_service);

  // update_client::Configurator overrides.
  int InitialDelay() const override;
  int NextCheckDelay() const override;
  int OnDemandDelay() const override;
  int UpdateDelay() const override;
  std::vector<GURL> UpdateUrl() const override;
  std::vector<GURL> PingUrl() const override;
  std::string GetProdId() const override;
  base::Version GetBrowserVersion() const override;
  std::string GetChannel() const override;
  std::string GetBrand() const override;
  std::string GetLang() const override;
  std::string GetOSLongName() const override;
  base::flat_map<std::string, std::string> ExtraRequestParams() const override;
  std::string GetDownloadPreference() const override;
  scoped_refptr<update_client::NetworkFetcherFactory> GetNetworkFetcherFactory()
      override;
  std::unique_ptr<service_manager::Connector> CreateServiceManagerConnector()
      const override;
  bool EnabledDeltas() const override;
  bool EnabledComponentUpdates() const override;
  bool EnabledBackgroundDownloader() const override;
  bool EnabledCupSigning() const override;
  PrefService* GetPrefService() const override;
  update_client::ActivityDataService* GetActivityDataService() const override;
  bool IsPerUserInstall() const override;
  std::vector<uint8_t> GetRunActionKeyHash() const override;
  std::string GetAppGuid() const override;
  std::unique_ptr<update_client::ProtocolHandlerFactory>
      GetProtocolHandlerFactory() const override;
  update_client::RecoveryCRXElevator GetRecoveryCRXElevator() const override;

 private:
  friend class base::RefCountedThreadSafe<BraveConfigurator>;

  ConfiguratorImpl configurator_impl_;
  PrefService* pref_service_;  // This member is not owned by this class.
  scoped_refptr<update_client::NetworkFetcherFactory> network_fetcher_factory_;

  ~BraveConfigurator() override {}
};

// Allows the component updater to use non-encrypted communication with the
// update backend. The security of the update checks is enforced using
// a custom message signing protocol and it does not depend on using HTTPS.
BraveConfigurator::BraveConfigurator(
    const base::CommandLine* cmdline,
    PrefService* pref_service)
    : configurator_impl_(ComponentUpdaterCommandLineConfigPolicy(cmdline),
        false),
      pref_service_(pref_service) {
  DCHECK(pref_service_);
}

int BraveConfigurator::InitialDelay() const {
  return configurator_impl_.InitialDelay();
}

int BraveConfigurator::NextCheckDelay() const {
  return configurator_impl_.NextCheckDelay();
}

int BraveConfigurator::OnDemandDelay() const {
  return configurator_impl_.OnDemandDelay();
}

int BraveConfigurator::UpdateDelay() const {
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

std::string BraveConfigurator::GetBrand() const {
  return std::string();
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
            g_browser_process->system_network_context_manager()
                ->GetSharedURLLoaderFactory());
  }
  return network_fetcher_factory_;
}

std::unique_ptr<service_manager::Connector>
BraveConfigurator::CreateServiceManagerConnector() const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  return content::ServiceManagerConnection::GetForProcess()
      ->GetConnector()
      ->Clone();
}

bool BraveConfigurator::EnabledDeltas() const {
  return configurator_impl_.EnabledDeltas();
}

bool BraveConfigurator::EnabledComponentUpdates() const {
  return pref_service_->GetBoolean(prefs::kComponentUpdatesEnabled);
}

bool BraveConfigurator::EnabledBackgroundDownloader() const {
  return configurator_impl_.EnabledBackgroundDownloader();
}

bool BraveConfigurator::EnabledCupSigning() const {
  return false;
}

PrefService* BraveConfigurator::GetPrefService() const {
  return pref_service_;
}

update_client::ActivityDataService* BraveConfigurator::GetActivityDataService()
    const {
  return nullptr;
}

bool BraveConfigurator::IsPerUserInstall() const {
  return false;
}

std::vector<uint8_t> BraveConfigurator::GetRunActionKeyHash() const {
  return configurator_impl_.GetRunActionKeyHash();
}

std::string BraveConfigurator::GetAppGuid() const {
  return configurator_impl_.GetAppGuid();
}

std::unique_ptr<update_client::ProtocolHandlerFactory>
BraveConfigurator::GetProtocolHandlerFactory() const {
  return std::make_unique<update_client::ProtocolHandlerFactoryXml>();
}

update_client::RecoveryCRXElevator BraveConfigurator::GetRecoveryCRXElevator()
    const {
#if defined(GOOGLE_CHROME_BUILD) && defined(OS_WIN)
  return base::BindOnce(&RunRecoveryCRXElevated);
#else
  return {};
#endif
}

}  // namespace

void RegisterPrefsForBraveComponentUpdaterConfigurator(
    PrefRegistrySimple* registry) {
  // The component updates are enabled by default, if the preference is not set.
  registry->RegisterBooleanPref(prefs::kComponentUpdatesEnabled, true);
}

scoped_refptr<update_client::Configurator>
MakeBraveComponentUpdaterConfigurator(
    const base::CommandLine* cmdline,
    PrefService* pref_service) {
  return base::MakeRefCounted<BraveConfigurator>(cmdline, pref_service);
}

}  // namespace component_updater
