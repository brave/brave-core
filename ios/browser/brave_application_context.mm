#import "brave/ios/browser/brave_application_context.h"
#include "brave/ios/browser/prefs/browser_prefs.h"
#include "brave/vendor/brave-ios/components/browser_state/chrome_browser_state.h"

#include <algorithm>
#include <vector>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/path_service.h"
#include "base/sequenced_task_runner.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"
#include "base/time/default_clock.h"
#include "base/time/default_tick_clock.h"

#include "components/autofill/core/common/autofill_prefs.h"
#include "components/browsing_data/core/pref_names.h"
#include "components/feed/core/shared_prefs/pref_names.h"
#include "components/flags_ui/pref_service_flags_storage.h"
#include "components/history/core/common/pref_names.h"
#include "components/language/core/browser/language_prefs.h"
#include "components/language/core/browser/pref_names.h"
#include "components/metrics/metrics_pref_names.h"
#include "components/payments/core/payment_prefs.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/proxy_config/pref_proxy_config_tracker_impl.h"
#include "components/safe_browsing/core/common/safe_browsing_prefs.h"
#include "components/sessions/core/session_id_generator.h"
#include "components/signin/public/base/signin_pref_names.h"
#include "components/sync/base/sync_prefs.h"
#include "components/sync_device_info/device_info_prefs.h"
#include "components/sync_sessions/session_sync_prefs.h"
#include "components/translate/core/browser/translate_pref_names.h"
#include "components/translate/core/browser/translate_prefs.h"

#include "components/history/core/browser/history_service.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/update_client/update_client.h"
#include "components/variations/service/variations_service.h"
#include "components/web_resource/web_resource_pref_names.h"

#include "ios/chrome/browser/application_context.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state_manager_impl.h"
#include "ios/chrome/browser/chrome_paths.h"
#include "ios/chrome/browser/history/history_service_factory.h"
#include "ios/chrome/browser/ios_chrome_io_thread.h"
#include "ios/chrome/browser/pref_names.h"
#include "ios/chrome/browser/prefs/browser_prefs.h"
#include "ios/chrome/browser/prefs/ios_chrome_pref_service_factory.h"
#include "ios/chrome/browser/browser_state/browser_state_info_cache.h"
#include "ios/chrome/common/channel_info.h"
#include "ios/web/public/thread/web_task_traits.h"
#include "ios/web/public/thread/web_thread.h"
#include "ios/chrome/browser/net/ios_chrome_network_delegate.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "net/log/net_log.h"
#include "net/log/net_log_capture_mode.h"
#include "net/socket/client_socket_pool_manager.h"
#include "net/url_request/url_request_context_getter.h"
#include "services/metrics/public/cpp/ukm_recorder.h"
#include "services/network/network_change_manager.h"
#include "services/network/public/cpp/network_connection_tracker.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/mojom/network_service.mojom.h"
#include "ui/base/resource/resource_bundle.h"

namespace {
// Passed to NetworkConnectionTracker to bind a NetworkChangeManager receiver.
void BindNetworkChangeManagerReceiver(
    network::NetworkChangeManager* network_change_manager,
    mojo::PendingReceiver<network::mojom::NetworkChangeManager> receiver) {
  network_change_manager->AddReceiver(std::move(receiver));
}
}  // namespace

BraveApplicationContext::BraveApplicationContext(
    base::SequencedTaskRunner* local_state_task_runner,
    const base::CommandLine& command_line,
    const std::string& locale)
    : local_state_task_runner_(local_state_task_runner),
      was_last_shutdown_clean_(false) {
  DCHECK(!GetApplicationContext());
  SetApplicationContext(this);

  SetApplicationLocale(locale);
}

BraveApplicationContext::~BraveApplicationContext() {
  DCHECK_EQ(this, GetApplicationContext());
  SetApplicationContext(nullptr);
}

void BraveApplicationContext::PreCreateThreads() {
  DCHECK(thread_checker_.CalledOnValidThread());
  ios_chrome_io_thread_.reset(
      new IOSChromeIOThread(GetLocalState(), GetNetLog()));
}

void BraveApplicationContext::PreMainMessageLoopRun() {
  DCHECK(thread_checker_.CalledOnValidThread());
}

void BraveApplicationContext::StartTearDown() {
  DCHECK(thread_checker_.CalledOnValidThread());

  // Need to clear browser states before the IO thread.
  chrome_browser_state_manager_.reset();

  if (local_state_) {
    local_state_->CommitPendingWrite();
    sessions::SessionIdGenerator::GetInstance()->Shutdown();
  }

  ios_chrome_io_thread_->NetworkTearDown();
}

void BraveApplicationContext::PostDestroyThreads() {
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

void BraveApplicationContext::OnAppEnterForeground() {
  DCHECK(thread_checker_.CalledOnValidThread());

  PrefService* local_state = GetLocalState();
  local_state->SetBoolean(prefs::kLastSessionExitedCleanly, false);
}

void BraveApplicationContext::OnAppEnterBackground() {
  DCHECK(thread_checker_.CalledOnValidThread());
    
    //TODO: FIX BRANDON
  // Mark all the ChromeBrowserStates as clean and persist history.
  /*std::vector<ChromeBrowserState*> loaded_browser_state =
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
  }*/

  PrefService* local_state = GetLocalState();
  local_state->SetBoolean(prefs::kLastSessionExitedCleanly, true);

  // Persisting to disk is protected by a critical task, so no other special
  // handling is necessary on iOS.
}

bool BraveApplicationContext::WasLastShutdownClean() {
  DCHECK(thread_checker_.CalledOnValidThread());
  // Make sure the locale state is created as the file is initialized there.
  ignore_result(GetLocalState());
  return was_last_shutdown_clean_;
}

PrefService* BraveApplicationContext::GetLocalState() {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!local_state_)
    CreateLocalState();
  return local_state_.get();
}

net::URLRequestContextGetter*
BraveApplicationContext::GetSystemURLRequestContext() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return ios_chrome_io_thread_->system_url_request_context_getter();
}

scoped_refptr<network::SharedURLLoaderFactory>
BraveApplicationContext::GetSharedURLLoaderFactory() {
  return ios_chrome_io_thread_->GetSharedURLLoaderFactory();
}

network::mojom::NetworkContext*
BraveApplicationContext::GetSystemNetworkContext() {
  return ios_chrome_io_thread_->GetSystemNetworkContext();
}

const std::string& BraveApplicationContext::GetApplicationLocale() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(!application_locale_.empty());
  return application_locale_;
}

ios::ChromeBrowserStateManager*
BraveApplicationContext::GetChromeBrowserStateManager() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return nullptr;
    
    //TODO: BRANDON FIX
//  if (!chrome_browser_state_manager_)
//    chrome_browser_state_manager_.reset(new ChromeBrowserStateManagerImpl());
//  return chrome_browser_state_manager_.get();
}

metrics_services_manager::MetricsServicesManager*
BraveApplicationContext::GetMetricsServicesManager() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return nullptr;
}

metrics::MetricsService* BraveApplicationContext::GetMetricsService() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return nullptr;
}

ukm::UkmRecorder* BraveApplicationContext::GetUkmRecorder() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return nullptr;
}

variations::VariationsService* BraveApplicationContext::GetVariationsService() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return nullptr;
}

rappor::RapporServiceImpl* BraveApplicationContext::GetRapporServiceImpl() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return nullptr;
}

net::NetLog* BraveApplicationContext::GetNetLog() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return nullptr;
}

net_log::NetExportFileWriter* BraveApplicationContext::GetNetExportFileWriter() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return nullptr;
}

network_time::NetworkTimeTracker*
BraveApplicationContext::GetNetworkTimeTracker() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return nullptr;
}

IOSChromeIOThread* BraveApplicationContext::GetIOSChromeIOThread() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(ios_chrome_io_thread_.get());
  return ios_chrome_io_thread_.get();
}

gcm::GCMDriver* BraveApplicationContext::GetGCMDriver() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return nullptr;
}

component_updater::ComponentUpdateService*
BraveApplicationContext::GetComponentUpdateService() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return nullptr;
}

SafeBrowsingService* BraveApplicationContext::GetSafeBrowsingService() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return nullptr;
}

network::NetworkConnectionTracker*
BraveApplicationContext::GetNetworkConnectionTracker() {
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

BrowserPolicyConnectorIOS* BraveApplicationContext::GetBrowserPolicyConnector() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return nullptr;
}

void BraveApplicationContext::SetApplicationLocale(const std::string& locale) {
  DCHECK(thread_checker_.CalledOnValidThread());
  application_locale_ = locale;
}

void BraveApplicationContext::CreateLocalState() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(!local_state_);

  base::FilePath local_state_path;
  CHECK(base::PathService::Get(ios::FILE_LOCAL_STATE, &local_state_path));
  scoped_refptr<PrefRegistrySimple> pref_registry(new PrefRegistrySimple);

  // Register local state preferences.
  RegisterLocalStatePrefs(pref_registry.get());

  policy::BrowserPolicyConnector* browser_policy_connector = nullptr;
  policy::PolicyService* policy_service = nullptr;
  local_state_ = ::CreateLocalState(
      local_state_path, local_state_task_runner_.get(), pref_registry,
      policy_service, browser_policy_connector);
  DCHECK(local_state_);

  sessions::SessionIdGenerator::GetInstance()->Init(local_state_.get());

  net::ClientSocketPoolManager::set_max_sockets_per_proxy_server(
      net::HttpNetworkSession::NORMAL_SOCKET_POOL,
      std::max(std::min<int>(net::kDefaultMaxSocketsPerProxyServer, 99),
               net::ClientSocketPoolManager::max_sockets_per_group(
                   net::HttpNetworkSession::NORMAL_SOCKET_POOL)));

  // Register the shutdown state before anything changes it.
  if (local_state_->HasPrefPath(prefs::kLastSessionExitedCleanly)) {
    was_last_shutdown_clean_ =
        local_state_->GetBoolean(prefs::kLastSessionExitedCleanly);
  }
}

void BraveApplicationContext::CreateGCMDriver() {
  DCHECK(thread_checker_.CalledOnValidThread());
}
