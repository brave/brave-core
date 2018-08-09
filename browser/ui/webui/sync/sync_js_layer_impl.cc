#include "brave/browser/ui/webui/sync/sync_js_layer_impl.h"

#include "base/bind.h"
#include "base/values.h"
#include "content/public/browser/web_ui_message_handler.h"

#include "brave/components/brave_sync/brave_sync_controller.h"
#include "brave/components/brave_sync/brave_sync_controller_impl.h"
#include "brave/components/brave_sync/value_debug.h"

SyncJsLayerImpl::SyncJsLayerImpl(content::WebUI* web_ui, const std::string& host,
    const std::string& js_file, int js_resource_id, int html_resource_id)
    : BasicUI(web_ui, host, js_file, js_resource_id, html_resource_id),
    init_js_lib_invoked_(false),
    page_loaded_(false),
    need_init_js_lib_(false) {

   RegisterCallbacks();

   //response_receiver_ = brave_sync::BraveSyncControllerImpl::GetInstance();
   sync_lib_to_browser_handler_ = brave_sync::BraveSyncControllerImpl::GetInstance();
   brave_sync::BraveSyncControllerImpl::GetInstance()->SetupJsLayer(this);
}

SyncJsLayerImpl::~SyncJsLayerImpl() {
}

void SyncJsLayerImpl::RegisterCallbacks() {
  this->web_ui()->RegisterMessageCallback("pageInitialized",
     base::Bind(&SyncJsLayerImpl::PageInitialized,
                base::Unretained(this)));

  this->web_ui()->RegisterMessageCallback("HandleMessage",
     base::Bind(&SyncJsLayerImpl::HandleMessage,
                base::Unretained(this)));
}

// Sent by brave_sync_lib.js code when page completed to load
void SyncJsLayerImpl::PageInitialized(const base::ListValue* args) {
  page_loaded_ = true;
  if (need_init_js_lib_) {
    web_ui()->CallJavascriptFunctionUnsafe("sync_lib_exports.loadJsLibScript");
    init_js_lib_invoked_ = true;
  }
}

void SyncJsLayerImpl::LoadJsLibScript() {
  DCHECK(init_js_lib_invoked_ == false);
  if (page_loaded_) {
    web_ui()->CallJavascriptFunctionUnsafe("sync_lib_exports.loadJsLibScript");
    init_js_lib_invoked_ = true;
      // ^- when is called right after browser start
      // [41447:41447:0703/141816.983776:ERROR:CONSOLE(1)] "Uncaught ReferenceError: hello_world is not defined", source:  (1)
  } else {
    DCHECK(need_init_js_lib_ == false);
    need_init_js_lib_ = true;
  }
}

void SyncJsLayerImpl::HandleMessage(const base::ListValue* args) {
LOG(ERROR) << "TAGAB SyncJsLayerImpl::HandleMessage";
LOG(ERROR) << "TAGAB args->GetSize()="<<args->GetSize();
LOG(ERROR) << "TAGAB args->GetList()[0].GetString()="<<args->GetList()[0].GetString();

for (const auto &val : args->GetList() ) {
  LOG(ERROR) << "TAGAB val.type()="<< base::Value::GetTypeName(val.type());
}

const std::string message = args->GetList()[0].GetString();
if (message == "words_to_bytes_done" || message == "bytes_to_words_done") {
  LOG(ERROR) << "TAGAB args[1].GetString()="<<args->GetList()[1];
} else {
  LOG(ERROR) << "TAGAB args[1].GetString()="<<args->GetList()[1].GetString();
  LOG(ERROR) << "TAGAB args[2].GetString()="<<args->GetList()[2];//.GetString();
  LOG(ERROR) << "TAGAB args[3].GetString()="<<args->GetList()[3].GetString();
  LOG(ERROR) << "TAGAB args[4].GetBool()="<<args->GetList()[4].GetBool();
}

  // if (response_receiver_) {
  //   response_receiver_->OnJsLibMessage(message, args);
  // }
  if (sync_lib_to_browser_handler_) {
    sync_lib_to_browser_handler_->OnJsLibMessage(message, args);
  }
}

void SyncJsLayerImpl::RunCommandBV(const std::vector<const base::Value*> &args) {
  LOG(ERROR) << "TAGAB SyncJsLayerImpl::RunCommandBV";
  DCHECK(args.size() >= 5);
  LOG(ERROR) << "TAGAB SyncJsLayerImpl::RunCommandBV args[0]=" << brave::debug::ToPrintableString( *args.at(0));
  LOG(ERROR) << "TAGAB SyncJsLayerImpl::RunCommandBV args[1]=" << brave::debug::ToPrintableString( *args.at(1));
  LOG(ERROR) << "TAGAB SyncJsLayerImpl::RunCommandBV args[2]=" << brave::debug::ToPrintableString( *args.at(2));
  LOG(ERROR) << "TAGAB SyncJsLayerImpl::RunCommandBV args[3]=" << brave::debug::ToPrintableString( *args.at(3));
  LOG(ERROR) << "TAGAB SyncJsLayerImpl::RunCommandBV args[4]=" << brave::debug::ToPrintableString( *args.at(4));
  web_ui()->CallJavascriptFunctionUnsafe("sync_lib_exports.callJsLib", args);
}

void SyncJsLayerImpl::RunCommandStr(const std::string &command,
  const std::string &arg1, const std::string &arg2, const std::string &arg3,
  const std::string &arg4) {
  LOG(ERROR) << "TAGAB SyncJsLayerImpl::RunCommandStr command=<"<<command<<">";
  LOG(ERROR) << "TAGAB SyncJsLayerImpl::RunCommandStr arg1=<"<<arg1<<">";
  LOG(ERROR) << "TAGAB SyncJsLayerImpl::RunCommandStr arg2=<"<<arg2<<">";
  LOG(ERROR) << "TAGAB SyncJsLayerImpl::RunCommandStr arg3=<"<<arg3<<">";
  LOG(ERROR) << "TAGAB SyncJsLayerImpl::RunCommandStr arg4=<"<<arg4<<">";

  // Messages marked as browser -> webview from braveSync/client/constants/messages.js
  DCHECK(command == "got-init-data" ||
    command == "words_to_bytes" ||
    command == "bytes_to_words" ||
    command == "send-sync-records"  ||
    command == "fetch-sync-records"  ||
    command == "fetch-sync-devices"  ||
    command == "resolve-sync-records"  ||
    command == "send-sync-records"  ||
    command == "delete-sync-user"  ||
    command == "delete-sync-category"  ||
    command == "get-bookmarks-base-order"  ||
    command == "get-bookmark-order" );

  base::Value bv_command(command);
  base::Value bv_arg1(arg1);
  base::Value bv_arg2(arg2);
  base::Value bv_arg3(arg3);
  base::Value bv_arg4(arg4);

  std::vector<const base::Value*> args = { &bv_command, &bv_arg1, &bv_arg2, &bv_arg3, &bv_arg4};
  web_ui()->CallJavascriptFunctionUnsafe("sync_lib_exports.callJsLibStr", args);
}
