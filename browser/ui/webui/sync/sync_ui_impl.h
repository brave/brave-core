#ifndef BRAVE_BROWSER_UI_WEBUI_SYNC_SYNC_UI_IMPL_H_
#define BRAVE_BROWSER_UI_WEBUI_SYNC_SYNC_UI_IMPL_H_

#include "base/macros.h"
#include "brave/browser/ui/webui/basic_ui.h"
#include "brave/browser/ui/webui/sync/sync_ui.h"

namespace base {
  class ListValue;
}  // namespace base

namespace brave_sync {
  class BraveSyncController;
} // namespace brave_sync

// The WebUI for chrome://bravesync
class SyncUIImpl : public BasicUI, //AB: should bot be inherited from BasicUI,
                   public SyncUI
{
  // because when OnPreferenceChanged is invoked, then in complains
  // brave_new_tab is not defined. It displays shields statistics and
  // is not required for sync
 public:
  explicit SyncUIImpl(content::WebUI* web_ui, const std::string& host,
      const std::string& js_file, int js_resource_id, int html_resource_id);
  ~SyncUIImpl() override;

  // should be invoked by controller when we know sync status has been changed
  void OnSyncStateChanged() override;
  void OnHaveSyncWords(const std::string &sync_words) override;
  void OnLogMessage(const std::string &message) override;

 private:
  void RegisterCallbacks();

  void SetupSyncHaveCode(const base::ListValue* args);
  void SetupSyncNewToSync(const base::ListValue* args);

  void PageLoaded(const base::ListValue* args);

  void NeedSyncWords(const base::ListValue* args);
  void NeedSyncQRcode(const base::ListValue* args);

  void DeleteDevice(const base::ListValue* args);
  void ResetSync(const base::ListValue* args);

  void TestClicked(const base::ListValue* args);

  // this should grab actual data from controller and update the page
  void LoadSyncSettingsView();

  brave_sync::BraveSyncController *sync_controller_;

  DISALLOW_COPY_AND_ASSIGN(SyncUIImpl);
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SYNC_SYNC_UI_H_
