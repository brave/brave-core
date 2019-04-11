/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "brave/browser/ui/webui/sync/sync_ui.h"

#include "base/bind.h"
#include "base/memory/weak_ptr.h"
#include "brave/common/webui_url_constants.h"
#include "brave/components/brave_sync/brave_sync_service.h"
#include "brave/components/brave_sync/brave_sync_service_observer.h"
#include "brave/components/brave_sync/grit/brave_sync_resources.h"
#include "brave/components/brave_sync/grit/brave_sync_generated_map.h"
#include "brave/components/brave_sync/sync_devices.h"
#include "brave/components/brave_sync/settings.h"
#include "brave/components/brave_sync/values_conv.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "components/browser_sync/profile_sync_service.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_ui_message_handler.h"

using content::WebUIMessageHandler;

namespace {

// The handler for Javascript messages for Brave about: pages
class SyncUIDOMHandler : public WebUIMessageHandler,
                         public brave_sync::BraveSyncServiceObserver {
 public:
  SyncUIDOMHandler();
  ~SyncUIDOMHandler() override;

  void Init();

  // WebUIMessageHandler implementation.
  void RegisterMessages() override;

 private:
  void SetupSyncHaveCode(const base::ListValue* args);
  void SetupSyncNewToSync(const base::ListValue* args);
  void PageLoaded(const base::ListValue* args);
  void NeedSyncWords(const base::ListValue* args);
  void NeedSyncQRcode(const base::ListValue* args);
  void SyncBookmarks(const base::ListValue* args);
  void SyncBrowsingHistory(const base::ListValue* args);
  void SyncSavedSiteSettings(const base::ListValue* args);
  void DeleteDevice(const base::ListValue* args);
  void ResetSync(const base::ListValue* args);

  void OnSyncSetupError(brave_sync::BraveSyncService* sync_service,
                        const std::string& error) override;
  void OnSyncStateChanged(brave_sync::BraveSyncService *sync_service) override;
  void OnHaveSyncWords(brave_sync::BraveSyncService *sync_service,
                       const std::string& sync_words) override;

  // this should grab actual data from controller and update the page
  void LoadSyncSettingsView();

  // Move to observer
  void GetSettingsAndDevicesComplete(
    std::unique_ptr<brave_sync::Settings> settings,
    std::unique_ptr<brave_sync::SyncDevices> devices);

  brave_sync::BraveSyncService* sync_service_ = nullptr;  // NOT OWNED

  base::WeakPtrFactory<SyncUIDOMHandler> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(SyncUIDOMHandler);
};

SyncUIDOMHandler::SyncUIDOMHandler() : weak_ptr_factory_(this) {}

SyncUIDOMHandler::~SyncUIDOMHandler() {
  if (sync_service_)
    sync_service_->RemoveObserver(this);
}

void SyncUIDOMHandler::RegisterMessages() {
  this->web_ui()->RegisterMessageCallback("setupSyncHaveCode",
     base::Bind(&SyncUIDOMHandler::SetupSyncHaveCode,
                base::Unretained(this)));

  this->web_ui()->RegisterMessageCallback("setupSyncNewToSync",
     base::Bind(&SyncUIDOMHandler::SetupSyncNewToSync,
                base::Unretained(this)));

  this->web_ui()->RegisterMessageCallback("pageLoaded",
     base::Bind(&SyncUIDOMHandler::PageLoaded,
                base::Unretained(this)));

  this->web_ui()->RegisterMessageCallback("needSyncWords",
     base::Bind(&SyncUIDOMHandler::NeedSyncWords,
                base::Unretained(this)));

  this->web_ui()->RegisterMessageCallback("needSyncQRcode",
     base::Bind(&SyncUIDOMHandler::NeedSyncQRcode,
                base::Unretained(this)));

  this->web_ui()->RegisterMessageCallback("syncBookmarks",
     base::Bind(&SyncUIDOMHandler::SyncBookmarks,
                base::Unretained(this)));

  this->web_ui()->RegisterMessageCallback("syncBrowsingHistory",
     base::Bind(&SyncUIDOMHandler::SyncBrowsingHistory,
                base::Unretained(this)));

  this->web_ui()->RegisterMessageCallback("syncSavedSiteSettings",
     base::Bind(&SyncUIDOMHandler::SyncSavedSiteSettings,
                base::Unretained(this)));

  this->web_ui()->RegisterMessageCallback("deleteDevice",
     base::Bind(&SyncUIDOMHandler::DeleteDevice,
                base::Unretained(this)));

  this->web_ui()->RegisterMessageCallback("resetSync",
     base::Bind(&SyncUIDOMHandler::ResetSync,
                base::Unretained(this)));
}

void SyncUIDOMHandler::Init() {
  Profile* profile = Profile::FromWebUI(web_ui());
  sync_service_ =
    static_cast<brave_sync::BraveSyncService*>(
      ProfileSyncServiceFactory::GetAsProfileSyncServiceForProfile(profile));
  if (sync_service_)
    sync_service_->AddObserver(this);
}

void SyncUIDOMHandler::SetupSyncHaveCode(const base::ListValue* args) {
  DCHECK(sync_service_);
  std::string sync_words, device_name;
  if (!args->GetString(0, &sync_words) || !args->GetString(1, &device_name))
    // TODO(bridiver) - does this need to report an error?
    return;

  sync_service_->OnSetupSyncHaveCode(sync_words, device_name);
}

void SyncUIDOMHandler::SetupSyncNewToSync(const base::ListValue* args) {
  DCHECK(sync_service_);
  std::string sync_words, device_name;
  if (!args->GetString(0, &device_name))
    // TODO(bridiver) - does this need to report an error?
    return;

  sync_service_->OnSetupSyncNewToSync(device_name);
}

void SyncUIDOMHandler::PageLoaded(const base::ListValue* args) {
  DCHECK(sync_service_);
  LoadSyncSettingsView();
}

void SyncUIDOMHandler::NeedSyncWords(const base::ListValue* args) {
  DCHECK(sync_service_);
  sync_service_->GetSyncWords();
}

void SyncUIDOMHandler::NeedSyncQRcode(const base::ListValue* args) {
  DCHECK(sync_service_);
  std::string seed = sync_service_->GetSeed();
  web_ui()->CallJavascriptFunctionUnsafe(
      "sync_ui_exports.haveSeedForQrCode", base::Value(seed));
}

void SyncUIDOMHandler::SyncBookmarks(const base::ListValue* args) {
  DCHECK(sync_service_);
  bool new_value;
  if (!args->GetBoolean(0, &new_value))
    // TODO(bridiver) - does this need to report an error?
    return;

  sync_service_->OnSetSyncBookmarks(new_value);
}

void SyncUIDOMHandler::SyncBrowsingHistory(const base::ListValue* args) {
  DCHECK(sync_service_);
  bool new_value;
  if (!args->GetBoolean(0, &new_value))
    // TODO(bridiver) - does this need to report an error?
    return;

  sync_service_->OnSetSyncBrowsingHistory(new_value);
}

void SyncUIDOMHandler::SyncSavedSiteSettings(const base::ListValue* args) {
  DCHECK(sync_service_);
  bool new_value;
  if (!args->GetBoolean(0, &new_value))
    // TODO(bridiver) - does this need to report an error?
    return;

  sync_service_->OnSetSyncSavedSiteSettings(new_value);
}

void SyncUIDOMHandler::DeleteDevice(const base::ListValue* args) {
  DCHECK(sync_service_);
  int i_device_id = -1;
  if (!args->GetInteger(0, &i_device_id) || i_device_id == -1) {
    DCHECK(false) << "[Brave Sync] could not get device id";
    return;
  }

  sync_service_->OnDeleteDevice(std::to_string(i_device_id));
}

void SyncUIDOMHandler::ResetSync(const base::ListValue* args) {
  DCHECK(sync_service_);
  sync_service_->OnResetSync();
}

void SyncUIDOMHandler::OnSyncSetupError(
    brave_sync::BraveSyncService* sync_service,
    const std::string& error) {

  web_ui()->CallJavascriptFunctionUnsafe(
      "sync_ui_exports.syncSetupError", base::Value(error));
}

void SyncUIDOMHandler::OnSyncStateChanged(
    brave_sync::BraveSyncService *sync_service) {
  LoadSyncSettingsView();
}

void SyncUIDOMHandler::LoadSyncSettingsView() {
  sync_service_->GetSettingsAndDevices(
      base::Bind(&SyncUIDOMHandler::GetSettingsAndDevicesComplete,
          weak_ptr_factory_.GetWeakPtr()));
}

void SyncUIDOMHandler::GetSettingsAndDevicesComplete(
  std::unique_ptr<brave_sync::Settings> settings,
  std::unique_ptr<brave_sync::SyncDevices> devices) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  std::unique_ptr<base::Value> bv_devices = devices->ToValueArrOnly();
  std::unique_ptr<base::Value> bv_settings =
      brave_sync::BraveSyncSettingsToValue(settings.get());
  web_ui()->CallJavascriptFunctionUnsafe(
      "sync_ui_exports.showSettings", *bv_settings, *bv_devices);
}

void SyncUIDOMHandler::OnHaveSyncWords(
    brave_sync::BraveSyncService *sync_service,
    const std::string& sync_words) {
  web_ui()->CallJavascriptFunctionUnsafe(
      "sync_ui_exports.haveSyncWords", base::Value(sync_words));
}

} // namespace

SyncUI::SyncUI(content::WebUI* web_ui, const std::string& name)
    : BasicUI(web_ui, name,
              kBraveSyncGenerated,
              kBraveSyncGeneratedSize,
              Profile::FromWebUI(web_ui)->IsOffTheRecord() ?
                  IDR_BRAVE_SYNC_DISABLED_HTML : IDR_BRAVE_SYNC_HTML) {
  auto handler_owner = std::make_unique<SyncUIDOMHandler>();
  SyncUIDOMHandler* handler = handler_owner.get();
  web_ui->AddMessageHandler(std::move(handler_owner));
  handler->Init();
}

SyncUI::~SyncUI() {
}
