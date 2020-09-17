//
//  brave_application_context.cpp
//  base/third_party/double_conversion:double_conversion
//
//  Created by brandon on 2020-05-12.
//

#include "brave/vendor/brave-ios/components/context/brave_application_context.h"
#include "brave/vendor/brave-ios/components/browser_state/chrome_browser_state.h"
#include "brave/vendor/brave-ios/components/browser_state/brave_browser_state_manager.h"

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
#include "base/time/default_clock.h"
#include "base/time/default_tick_clock.h"
#include "ios/chrome/browser/chrome_paths.h"
#include "ios/chrome/common/channel_info.h"
#include "ios/web/public/thread/web_task_traits.h"
#include "ios/web/public/thread/web_thread.h"

namespace brave {
BraveApplicationContext::BraveApplicationContext(
    base::SequencedTaskRunner* local_state_task_runner,
    const base::CommandLine& command_line,
    const std::string& locale)
    : local_state_task_runner_(local_state_task_runner),
      was_last_shutdown_clean_(false) {
  DCHECK(!GetApplicationContext());
  SetApplicationContext(this);

  SetApplicationLocale(locale);

  update_client::UpdateQueryParams::SetDelegate(
      IOSChromeUpdateQueryParamsDelegate::GetInstance());
}

BraveApplicationContext::~BraveApplicationContext() {
  DCHECK_EQ(this, GetApplicationContext());
  SetApplicationContext(nullptr);
}

void BraveApplicationContext::StartTearDown() {
  DCHECK(thread_checker_.CalledOnValidThread());

  chrome_browser_state_manager_.reset();
}

void BraveApplicationContext::PostDestroyThreads() {
  DCHECK(thread_checker_.CalledOnValidThread());
  ios_chrome_io_thread_.reset();
}

void BraveApplicationContext::OnAppEnterForeground() {
  DCHECK(thread_checker_.CalledOnValidThread());

  PrefService* local_state = GetLocalState();
  local_state->SetBoolean(prefs::kLastSessionExitedCleanly, false);
}

void BraveApplicationContext::OnAppEnterBackground() {
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

  PrefService* local_state = GetLocalState();
  local_state->SetBoolean(prefs::kLastSessionExitedCleanly, true);
}

bool BraveApplicationContext::WasLastShutdownClean() {
  DCHECK(thread_checker_.CalledOnValidThread());
  ignore_result(GetLocalState());
  return was_last_shutdown_clean_;
}

PrefService* BraveApplicationContext::GetLocalState() {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!local_state_)
    CreateLocalState();
  return local_state_.get();
}

const std::string& BraveApplicationContext::GetApplicationLocale() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(!application_locale_.empty());
  return application_locale_;
}

brave::ChromeBrowserStateManager*
BraveApplicationContext::GetChromeBrowserStateManager() {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!chrome_browser_state_manager_)
    chrome_browser_state_manager_.reset(new BraveBrowserStateManager());
  return chrome_browser_state_manager_.get();
}

IOSChromeIOThread* BraveApplicationContext::GetIOSChromeIOThread() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(ios_chrome_io_thread_.get());
  return ios_chrome_io_thread_.get();
}

void BraveApplicationContext::SetApplicationLocale(const std::string& locale) {
  DCHECK(thread_checker_.CalledOnValidThread());
  application_locale_ = locale;
  translate::TranslateDownloadManager::GetInstance()->set_application_locale(
      application_locale_);
}

void BraveApplicationContext::CreateLocalState() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(!local_state_);

  base::FilePath local_state_path;
  CHECK(base::PathService::Get(ios::FILE_LOCAL_STATE, &local_state_path));
  scoped_refptr<PrefRegistrySimple> pref_registry(new PrefRegistrySimple);

  // Register local state preferences.
  RegisterLocalStatePrefs(pref_registry.get());

  local_state_ = ::CreateLocalState(
      local_state_path, local_state_task_runner_.get(), pref_registry);
  DCHECK(local_state_);

  sessions::SessionIdGenerator::GetInstance()->Init(local_state_.get());

  // Register the shutdown state before anything changes it.
  if (local_state_->HasPrefPath(prefs::kLastSessionExitedCleanly)) {
    was_last_shutdown_clean_ =
        local_state_->GetBoolean(prefs::kLastSessionExitedCleanly);
  }
}
}
