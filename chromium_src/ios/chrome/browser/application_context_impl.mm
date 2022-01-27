/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ios/chrome/browser/application_context_impl.h"

#include <vector>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/memory/ptr_util.h"
#include "base/path_service.h"
#include "base/task/post_task.h"
#include "base/task/sequenced_task_runner.h"
#include "base/time/default_clock.h"
#include "base/time/default_tick_clock.h"
#include "brave/ios/browser/metrics/ios_brave_metrics_services_manager_client.h"
#include "components/breadcrumbs/core/breadcrumb_manager.h"
#include "components/breadcrumbs/core/breadcrumb_persistent_storage_manager.h"
#include "components/component_updater/component_updater_service.h"
#include "components/component_updater/timer_update_scheduler.h"
#include "components/gcm_driver/gcm_driver.h"
#include "components/history/core/browser/history_service.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/metrics/metrics_service.h"
#include "components/metrics_services_manager/metrics_services_manager.h"
#include "components/net_log/net_export_file_writer.h"
#include "components/network_time/network_time_tracker.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/sessions/core/session_id_generator.h"
#include "components/ukm/ukm_service.h"
#include "ios/chrome/browser/application_context.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state_manager_impl.h"
#include "ios/chrome/browser/chrome_paths.h"
#include "ios/chrome/browser/component_updater/ios_component_updater_configurator.h"
#import "ios/chrome/browser/crash_report/breadcrumbs/application_breadcrumbs_logger.h"
#include "ios/chrome/browser/history/history_service_factory.h"
#include "ios/chrome/browser/ios_chrome_io_thread.h"
#include "ios/chrome/browser/policy/browser_policy_connector_ios.h"
#include "ios/chrome/browser/pref_names.h"
#include "ios/chrome/browser/prefs/browser_prefs.h"
#include "ios/chrome/browser/prefs/ios_chrome_pref_service_factory.h"
#import "ios/chrome/browser/safe_browsing/safe_browsing_service.h"
#include "ios/public/provider/chrome/browser/app_distribution/app_distribution_api.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "net/log/net_log.h"
#include "net/log/net_log_capture_mode.h"
#include "net/socket/client_socket_pool_manager.h"
#include "net/url_request/url_request_context_getter.h"
#include "services/metrics/public/cpp/ukm_recorder.h"
#include "services/network/network_change_manager.h"
#include "services/network/public/cpp/network_connection_tracker.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// Passed to NetworkConnectionTracker to bind a NetworkChangeManager receiver.
void BindNetworkChangeManagerReceiver(
    network::NetworkChangeManager* network_change_manager,
    mojo::PendingReceiver<network::mojom::NetworkChangeManager> receiver) {
  network_change_manager->AddReceiver(std::move(receiver));
}

}  // namespace

ApplicationContextImpl::ApplicationContextImpl(
    base::SequencedTaskRunner* local_state_task_runner,
    const base::CommandLine& command_line,
    const std::string& locale)
    : local_state_task_runner_(local_state_task_runner) {
  DCHECK(!GetApplicationContext());
  SetApplicationContext(this);

  SetApplicationLocale(locale);

  // update_client::UpdateQueryParams::SetDelegate(
  //     IOSChromeUpdateQueryParamsDelegate::GetInstance());
}

ApplicationContextImpl::~ApplicationContextImpl() {
  DCHECK_EQ(this, GetApplicationContext());
  SetApplicationContext(nullptr);
}

void ApplicationContextImpl::PreCreateThreads() {
  DCHECK(thread_checker_.CalledOnValidThread());
  ios_chrome_io_thread_.reset(
      new IOSChromeIOThread(GetLocalState(), GetNetLog()));
}

void ApplicationContextImpl::PreMainMessageLoopRun() {
  DCHECK(thread_checker_.CalledOnValidThread());
}

void ApplicationContextImpl::StartTearDown() {
  DCHECK(thread_checker_.CalledOnValidThread());
  // We need to destroy the NetworkTimeTracker before the IO thread gets
  // destroyed, since the destructor can call the URLFetcher destructor,
  // which does a PostDelayedTask operation on the IO thread. (The IO thread
  // will handle that URLFetcher operation before going away.)
  network_time_tracker_.reset();

  net_export_file_writer_.reset();

  // Need to clear browser states before the IO thread.
  chrome_browser_state_manager_.reset();

  if (local_state_) {
    local_state_->CommitPendingWrite();
    sessions::SessionIdGenerator::GetInstance()->Shutdown();
  }

  ios_chrome_io_thread_->NetworkTearDown();
}

void ApplicationContextImpl::PostDestroyThreads() {
  DCHECK(thread_checker_.CalledOnValidThread());
  // Resets associated state right after actual thread is stopped as
  // IOSChromeIOThread::Globals cleanup happens in CleanUp on the IO
  // thread, i.e. as the thread exits its message loop.
  //
  // This is important because in various places, the IOSChromeIOThread
  // object being NULL is considered synonymous with the IO thread
  // having stopped.
  ios_chrome_io_thread_.reset();
}

void ApplicationContextImpl::OnAppEnterForeground() {
  DCHECK(thread_checker_.CalledOnValidThread());
}

void ApplicationContextImpl::OnAppEnterBackground() {
  DCHECK(thread_checker_.CalledOnValidThread());
  // Mark all the ChromeBrowserStates as clean and persist history.
  std::vector<ChromeBrowserState*> loaded_browser_state =
      GetChromeBrowserStateManager()->GetLoadedBrowserStates();
  for (ChromeBrowserState* browser_state : loaded_browser_state) {
    if (history::HistoryService* history_service =
            ios::HistoryServiceFactory::GetForBrowserStateIfExists(
                browser_state, ServiceAccessType::EXPLICIT_ACCESS)) {
      history_service->HandleBackgrounding();
    }

    PrefService* browser_state_prefs = browser_state->GetPrefs();
    if (browser_state_prefs)
      browser_state_prefs->CommitPendingWrite();
  }
}

bool ApplicationContextImpl::WasLastShutdownClean() {
  DCHECK(thread_checker_.CalledOnValidThread());
  // Make sure the locale state is created as the file is initialized there.
  return true;
}

PrefService* ApplicationContextImpl::GetLocalState() {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!local_state_)
    CreateLocalState();
  return local_state_.get();
}

net::URLRequestContextGetter*
ApplicationContextImpl::GetSystemURLRequestContext() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return ios_chrome_io_thread_->system_url_request_context_getter();
}

scoped_refptr<network::SharedURLLoaderFactory>
ApplicationContextImpl::GetSharedURLLoaderFactory() {
  return ios_chrome_io_thread_->GetSharedURLLoaderFactory();
}

network::mojom::NetworkContext*
ApplicationContextImpl::GetSystemNetworkContext() {
  return ios_chrome_io_thread_->GetSystemNetworkContext();
}

const std::string& ApplicationContextImpl::GetApplicationLocale() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(!application_locale_.empty());
  return application_locale_;
}

ios::ChromeBrowserStateManager*
ApplicationContextImpl::GetChromeBrowserStateManager() {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!chrome_browser_state_manager_)
    chrome_browser_state_manager_.reset(new ChromeBrowserStateManagerImpl());
  return chrome_browser_state_manager_.get();
}

metrics_services_manager::MetricsServicesManager*
ApplicationContextImpl::GetMetricsServicesManager() {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!metrics_services_manager_) {
    metrics_services_manager_.reset(
        new metrics_services_manager::MetricsServicesManager(
            std::make_unique<IOSBraveMetricsServicesManagerClient>(
                GetLocalState())));
  }
  return metrics_services_manager_.get();
}

metrics::MetricsService* ApplicationContextImpl::GetMetricsService() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return GetMetricsServicesManager()->GetMetricsService();
}

ukm::UkmRecorder* ApplicationContextImpl::GetUkmRecorder() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return GetMetricsServicesManager()->GetUkmService();
}

variations::VariationsService* ApplicationContextImpl::GetVariationsService() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return GetMetricsServicesManager()->GetVariationsService();
}

net::NetLog* ApplicationContextImpl::GetNetLog() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return net::NetLog::Get();
}

net_log::NetExportFileWriter* ApplicationContextImpl::GetNetExportFileWriter() {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!net_export_file_writer_) {
    net_export_file_writer_ = std::make_unique<net_log::NetExportFileWriter>();
  }
  return net_export_file_writer_.get();
}

network_time::NetworkTimeTracker*
ApplicationContextImpl::GetNetworkTimeTracker() {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!network_time_tracker_) {
    network_time_tracker_.reset(new network_time::NetworkTimeTracker(
        base::WrapUnique(new base::DefaultClock),
        base::WrapUnique(new base::DefaultTickClock), GetLocalState(),
        GetSharedURLLoaderFactory()));
  }
  return network_time_tracker_.get();
}

IOSChromeIOThread* ApplicationContextImpl::GetIOSChromeIOThread() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(ios_chrome_io_thread_.get());
  return ios_chrome_io_thread_.get();
}

gcm::GCMDriver* ApplicationContextImpl::GetGCMDriver() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return nullptr;
}

component_updater::ComponentUpdateService*
ApplicationContextImpl::GetComponentUpdateService() {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!component_updater_) {
    // Creating the component updater does not do anything, components need to
    // be registered and Start() needs to be called.
    component_updater_ = component_updater::ComponentUpdateServiceFactory(
        component_updater::MakeIOSComponentUpdaterConfigurator(
            base::CommandLine::ForCurrentProcess()),
        std::make_unique<component_updater::TimerUpdateScheduler>(),
        ios::provider::GetBrandCode());
  }
  return component_updater_.get();
}

SafeBrowsingService* ApplicationContextImpl::GetSafeBrowsingService() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return nullptr;
}

network::NetworkConnectionTracker*
ApplicationContextImpl::GetNetworkConnectionTracker() {
  if (!network_connection_tracker_) {
    if (!network_change_manager_) {
      network_change_manager_ =
          std::make_unique<network::NetworkChangeManager>(nullptr);
    }
    network_connection_tracker_ =
        std::make_unique<network::NetworkConnectionTracker>(base::BindRepeating(
            &BindNetworkChangeManagerReceiver,
            base::Unretained(network_change_manager_.get())));
  }
  return network_connection_tracker_.get();
}

BrowserPolicyConnectorIOS* ApplicationContextImpl::GetBrowserPolicyConnector() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return nullptr;
}

breadcrumbs::BreadcrumbPersistentStorageManager*
ApplicationContextImpl::GetBreadcrumbPersistentStorageManager() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return nullptr;
}

void ApplicationContextImpl::SetApplicationLocale(const std::string& locale) {
  DCHECK(thread_checker_.CalledOnValidThread());
  application_locale_ = locale;
}

void ApplicationContextImpl::CreateLocalState() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(!local_state_);

  base::FilePath local_state_path;
  CHECK(base::PathService::Get(ios::FILE_LOCAL_STATE, &local_state_path));
  scoped_refptr<PrefRegistrySimple> pref_registry(new PrefRegistrySimple);

  // Register local state preferences.
  RegisterLocalStatePrefs(pref_registry.get());

  local_state_ = ::CreateLocalState(
      local_state_path, local_state_task_runner_.get(), pref_registry,
      nullptr, nullptr);
  DCHECK(local_state_);

  sessions::SessionIdGenerator::GetInstance()->Init(local_state_.get());

  net::ClientSocketPoolManager::set_max_sockets_per_proxy_server(
      net::HttpNetworkSession::NORMAL_SOCKET_POOL,
      std::max(std::min<int>(net::kDefaultMaxSocketsPerProxyServer, 99),
               net::ClientSocketPoolManager::max_sockets_per_group(
                   net::HttpNetworkSession::NORMAL_SOCKET_POOL)));
}

void ApplicationContextImpl::CreateGCMDriver() {
  DCHECK(thread_checker_.CalledOnValidThread());
}
