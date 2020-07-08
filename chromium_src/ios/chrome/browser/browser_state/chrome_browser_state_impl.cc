/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ios/chrome/browser/browser_state/chrome_browser_state_impl.h"

#include "build/build_config.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/threading/thread_restrictions.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/signin/public/base/signin_client.h"
#include "components/sync_preferences/pref_service_syncable.h"
#include "components/user_prefs/user_prefs.h"
#include "ios/chrome/browser/bookmarks/bookmark_model_factory.h"
#include "ios/chrome/browser/browser_state/bookmark_model_loaded_observer.h"
#include "ios/chrome/browser/prefs/browser_prefs.h"
#include "ios/chrome/browser/prefs/ios_chrome_pref_service_factory.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

// TODO(bridiver) temporary for ShellURLRequestContextGetter
#include "base/task/post_task.h"
#include "ios/web/public/thread/web_task_traits.h"
#include "ios/web/public/thread/web_thread.h"
#include "ios/web/shell/shell_url_request_context_getter.h"
#include "net/url_request/url_request_context_getter.h"

namespace {

// TODO(bridiver) - do we need to implement this?
class FakeSigninClient : public SigninClient {
 public:
  FakeSigninClient(ChromeBrowserState* browser_state)
      : browser_state_(browser_state) {}
  ~FakeSigninClient() override {}
  void Shutdown() override {}
  PrefService* GetPrefs() override {
    return browser_state_->GetPrefs();
  }
  scoped_refptr<network::SharedURLLoaderFactory>
  GetURLLoaderFactory() override {
    return browser_state_->GetSharedURLLoaderFactory();
  }
  network::mojom::CookieManager* GetCookieManager() override {
    return browser_state_->GetCookieManager();
  }
  void DoFinalInit() override {}
  bool AreSigninCookiesAllowed() override { return true; }
  bool AreSigninCookiesDeletedOnExit() override { return true; }
  void AddContentSettingsObserver(
      content_settings::Observer* observer) override {}
  void RemoveContentSettingsObserver(
      content_settings::Observer* observer) override {}
  void DelayNetworkCall(base::OnceClosure callback) override {}
  std::unique_ptr<GaiaAuthFetcher> CreateGaiaAuthFetcher(
      GaiaAuthConsumer* consumer,
      gaia::GaiaSource source) override {
    return std::unique_ptr<GaiaAuthFetcher>();
  }
  void PreGaiaLogout(base::OnceClosure callback) override {}

 private:
  ChromeBrowserState* browser_state_;
  DISALLOW_COPY_AND_ASSIGN(FakeSigninClient);
};

// Returns a bool indicating whether the necessary directories were able to be
// created (or already existed).
bool EnsureBrowserStateDirectoriesCreated(const base::FilePath& path) {
  // Create the browser state directory synchronously otherwise we would need to
  // sequence every otherwise independent I/O operation inside the browser state
  // directory with this operation. base::CreateDirectory() should be a
  // lightweight I/O operation and avoiding the headache of sequencing all
  // otherwise unrelated I/O after this one justifies running it on the main
  // thread.
  base::ThreadRestrictions::ScopedAllowIO allow_io_to_create_directory;

  if (!base::PathExists(path) && !base::CreateDirectory(path))
    return false;

  return true;
}

}  // namespace

ChromeBrowserStateImpl::ChromeBrowserStateImpl(
    scoped_refptr<base::SequencedTaskRunner> io_task_runner,
    const base::FilePath& path)
    : ChromeBrowserState(io_task_runner),
      state_path_(path),
      pref_registry_(new user_prefs::PrefRegistrySyncable),
      signin_client_(new FakeSigninClient(this)) {
  bool directories_created = EnsureBrowserStateDirectoriesCreated(state_path_);
  DCHECK(directories_created);

  RegisterBrowserStatePrefs(pref_registry_.get());
  // TODO(bridiver) - expose this through ApplicationContext
  // use the same registry for browser and local for now since we only have
  // one browser state anyway
  RegisterLocalStatePrefs(pref_registry_.get());

  BrowserStateDependencyManager::GetInstance()
      ->RegisterBrowserStatePrefsForServices(pref_registry_.get());
  prefs_ = CreateBrowserStatePrefs(state_path_,
                                   GetIOTaskRunner().get(),
                                   pref_registry_,
                                   nullptr,
                                   nullptr);

  // Register on BrowserState.
  user_prefs::UserPrefs::Set(this, prefs_.get());

  // Migrate obsolete prefs.
  // PrefService* local_state = GetApplicationContext()->GetLocalState();
  // MigrateObsoleteLocalStatePrefs(local_state);
  // MigrateObsoleteBrowserStatePrefs(prefs_.get());

  BrowserStateDependencyManager::GetInstance()->CreateBrowserStateServices(
      this);

  request_context_getter_ = new web::ShellURLRequestContextGetter(
      GetStatePath(), this,
      base::CreateSingleThreadTaskRunner({web::WebThread::IO}));

  // Listen for bookmark model load, to bootstrap the sync service.
  bookmarks::BookmarkModel* model =
      ios::BookmarkModelFactory::GetForBrowserState(this);
  model->AddObserver(new BookmarkModelLoadedObserver(this));
}

ChromeBrowserStateImpl::~ChromeBrowserStateImpl() {}

SigninClient* ChromeBrowserStateImpl::GetSigninClient() {
  return signin_client_.get();
}

ChromeBrowserState* ChromeBrowserStateImpl::GetOriginalChromeBrowserState() {
  return this;
}

bool ChromeBrowserStateImpl::HasOffTheRecordChromeBrowserState() const {
  return false;
}

ChromeBrowserState*
ChromeBrowserStateImpl::GetOffTheRecordChromeBrowserState() {
  return nullptr;
}

void ChromeBrowserStateImpl::DestroyOffTheRecordChromeBrowserState() {}

BrowserStatePolicyConnector* ChromeBrowserStateImpl::GetPolicyConnector() {
  return nullptr;
}

PrefService* ChromeBrowserStateImpl::GetPrefs() {
  DCHECK(prefs_);  // Should explicitly be initialized.
  return prefs_.get();
}

PrefService* ChromeBrowserStateImpl::GetOffTheRecordPrefs() {
  return nullptr;
}

ChromeBrowserStateIOData* ChromeBrowserStateImpl::GetIOData() {
  return nullptr;
}

void ChromeBrowserStateImpl::ClearNetworkingHistorySince(
    base::Time time,
    const base::Closure& completion) {}

PrefProxyConfigTracker* ChromeBrowserStateImpl::GetProxyConfigTracker() {
  return nullptr;
}

net::URLRequestContextGetter* ChromeBrowserStateImpl::CreateRequestContext(
    ProtocolHandlerMap* protocol_handlers) {
  return request_context_getter_.get();
}

bool ChromeBrowserStateImpl::IsOffTheRecord() const {
  return false;
}

base::FilePath ChromeBrowserStateImpl::GetStatePath() const {
  return state_path_;
}
