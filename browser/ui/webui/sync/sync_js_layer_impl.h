#ifndef BRAVE_BROWSER_UI_WEBUI_SYNC_SYNC_JS_LAYER_IMPL_H_
#define BRAVE_BROWSER_UI_WEBUI_SYNC_SYNC_JS_LAYER_IMPL_H_

#include "base/macros.h"
#include "brave/browser/ui/webui/basic_ui.h"
#include "brave/browser/ui/webui/sync/sync_js_layer.h"

namespace base {
  class ListValue;
  class Value;
}  // namespace base

class SyncJsLayerResponseReceiver;

// The WebUI for chrome://bravesynclib
class SyncJsLayerImpl : public BasicUI,
                    public SyncJsLayer { //AB: should not be inherited from BasicUI,
  // because when OnPreferenceChanged is invoked, then in complains
  // brave_new_tab is not defined. It displays shields statistics and not required for sync
public:
  explicit SyncJsLayerImpl(content::WebUI* web_ui, const std::string& host,
      const std::string& js_file, int js_resource_id, int html_resource_id);
  ~SyncJsLayerImpl() override;

private:
  void RegisterCallbacks();

  void PageInitialized(const base::ListValue* args);
  void HandleMessage(const base::ListValue* args);

  // SyncJsLayer virtual functions
  void LoadJsLibScript() override;
  void RunCommandBV(const std::vector<const base::Value*> &args) override;
  void RunCommandStr(const std::string &command,
    const std::string &arg1, const std::string &arg2, const std::string &arg3,
    const std::string &arg4) override;

  DISALLOW_COPY_AND_ASSIGN(SyncJsLayerImpl);

  SyncJsLayerResponseReceiver *response_receiver_; //TODO AB: the list of delegates?

  bool init_js_lib_invoked_;
  bool page_loaded_;
  bool need_init_js_lib_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SYNC_SYNC_JS_LAYER_IMPL_H_
