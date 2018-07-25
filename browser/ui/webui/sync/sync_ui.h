#ifndef BRAVE_BROWSER_UI_WEBUI_SYNC_I_SYNC_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_SYNC_I_SYNC_UI_H_

#include <string>

// TODO, AB: "Ixxxx" is not a Chromium style
// maybe use SyncUI and SyncUIimpl names?
// or delegates even better?

// This is need to send messages to SyncUI from SyncController
// (call methods of SyncUI from SyncController)
class SyncUI {
public:
  virtual ~SyncUI() {}
  // UI should pull all data and modify the page
  virtual void OnSyncStateChanged() = 0;
  virtual void OnHaveSyncWords(const std::string &sync_words) = 0;
  virtual void OnLogMessage(const std::string &message) = 0;
};

#endif //BRAVE_BROWSER_UI_WEBUI_SYNC_I_SYNC_UI_H_
