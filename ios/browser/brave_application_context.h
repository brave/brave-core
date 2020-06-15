#ifndef brave_application_context_h
#define brave_application_context_h

#include <memory>
#include <string>

#include "brave/ios/browser/application_context.h"

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/threading/thread_checker.h"

namespace base {
class CommandLine;
class SequencedTaskRunner;
}

namespace network {
class NetworkChangeManager;
}

class BraveApplicationContext : public ApplicationContext {
 public:
  BraveApplicationContext(base::SequencedTaskRunner* local_state_task_runner,
                         const base::CommandLine& command_line,
                         const std::string& locale);
  ~BraveApplicationContext() override;

  // Called before the browser threads are created.
  void PreCreateThreads();

  // Called after the threads have been created but before the message loops
  // starts running. Allows the ApplicationContext to do any initialization
  // that requres all threads running.
  void PreMainMessageLoopRun();

  // Most cleanup is done by these functions, driven from IOSChromeMainParts
  // rather than in the destructor, so that we can interleave cleanup with
  // threads being stopped.
  void StartTearDown();
  void PostDestroyThreads();

  // ApplicationContext implementation.
  void OnAppEnterForeground() override;
  void OnAppEnterBackground() override;
  bool WasLastShutdownClean() override;
  PrefService* GetLocalState() override;
  net::URLRequestContextGetter* GetSystemURLRequestContext() override;
  scoped_refptr<network::SharedURLLoaderFactory> GetSharedURLLoaderFactory()
      override;
  network::mojom::NetworkContext* GetSystemNetworkContext() override;
  const std::string& GetApplicationLocale() override;
  ios::ChromeBrowserStateManager* GetChromeBrowserStateManager() override;
  metrics_services_manager::MetricsServicesManager* GetMetricsServicesManager()
      override;
  metrics::MetricsService* GetMetricsService() override;
  ukm::UkmRecorder* GetUkmRecorder() override;
  variations::VariationsService* GetVariationsService() override;
  rappor::RapporServiceImpl* GetRapporServiceImpl() override;
  net::NetLog* GetNetLog() override;
  net_log::NetExportFileWriter* GetNetExportFileWriter() override;
  network_time::NetworkTimeTracker* GetNetworkTimeTracker() override;
  IOSChromeIOThread* GetIOSChromeIOThread() override;
  gcm::GCMDriver* GetGCMDriver() override;
  component_updater::ComponentUpdateService* GetComponentUpdateService()
      override;
  SafeBrowsingService* GetSafeBrowsingService() override;
  network::NetworkConnectionTracker* GetNetworkConnectionTracker() override;
  BrowserPolicyConnectorIOS* GetBrowserPolicyConnector() override;

 private:
  // Sets the locale used by the application.
  void SetApplicationLocale(const std::string& locale);

  // Create the local state.
  void CreateLocalState();
    
  // Create the gcm driver.
  void CreateGCMDriver();

  base::ThreadChecker thread_checker_;

  std::unique_ptr<PrefService> local_state_;
  std::unique_ptr<IOSChromeIOThread> ios_chrome_io_thread_;
  std::unique_ptr<ios::ChromeBrowserStateManager> chrome_browser_state_manager_;
  std::string application_locale_;

  // Sequenced task runner for local state related I/O tasks.
  const scoped_refptr<base::SequencedTaskRunner> local_state_task_runner_;
    
  std::unique_ptr<network::NetworkChangeManager> network_change_manager_;
  std::unique_ptr<network::NetworkConnectionTracker>
      network_connection_tracker_;

  bool was_last_shutdown_clean_;

  DISALLOW_COPY_AND_ASSIGN(BraveApplicationContext);
};

#endif
