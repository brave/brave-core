//
//  chrome_browser_state.cpp
//  base/third_party/double_conversion:double_conversion
//
//  Created by brandon on 2020-05-07.
//

#include "brave/vendor/brave-ios/components/browser_state/chrome_browser_state.h"

#include "ios/chrome/browser/chrome_constants.h"
#include "ios/chrome/browser/chrome_paths_internal.h"
#include "ios/chrome/browser/file_metadata_util.h"
#include "ios/chrome/browser/net/ios_chrome_url_request_context_getter.h"
#include "ios/chrome/browser/pref_names.h"
#include "ios/chrome/browser/prefs/browser_prefs.h"
#include "ios/chrome/browser/prefs/ios_chrome_pref_service_factory.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// All ChromeBrowserState will store a dummy base::SupportsUserData::Data
// object with this key. It can be used to check that a web::BrowserState
// is effectively a ChromeBrowserState when converting.
const char kBrowserStateIsChromeBrowserState[] = "IsChromeBrowserState";
}

namespace brave {
ChromeBrowserState::ChromeBrowserState(
    scoped_refptr<base::SequencedTaskRunner> io_task_runner)
    : io_task_runner_(std::move(io_task_runner)) {
  DCHECK(io_task_runner_);
  SetUserData(kBrowserStateIsChromeBrowserState,
              std::make_unique<base::SupportsUserData::Data>());
}

ChromeBrowserState::~ChromeBrowserState() {}

// static
ChromeBrowserState* ChromeBrowserState::FromBrowserState(
    web::BrowserState* browser_state) {
  if (!browser_state)
    return nullptr;

  // Check that the BrowserState is a ChromeBrowserState. It should always
  // be true in production and during tests as the only BrowserState that
  // should be used in ios/chrome inherits from ChromeBrowserState.
  DCHECK(browser_state->GetUserData(kBrowserStateIsChromeBrowserState));
  return static_cast<ChromeBrowserState*>(browser_state);
}

// static
ChromeBrowserState* ChromeBrowserState::FromWebUIIOS(web::WebUIIOS* web_ui) {
  return FromBrowserState(web_ui->GetWebState()->GetBrowserState());
}

std::string ChromeBrowserState::GetDebugName() {
  // The debug name is based on the state path of the original browser state
  // to keep in sync with the meaning on other platforms.
  std::string name =
      GetOriginalChromeBrowserState()->GetStatePath().BaseName().MaybeAsASCII();
  if (name.empty()) {
    name = "UnknownBrowserState";
  }
  return name;
}

scoped_refptr<base::SequencedTaskRunner> ChromeBrowserState::GetIOTaskRunner() {
  return io_task_runner_;
}

sync_preferences::PrefServiceSyncable* ChromeBrowserState::GetSyncablePrefs() {
  return static_cast<sync_preferences::PrefServiceSyncable*>(GetPrefs());
}

//net::URLRequestContextGetter* ChromeBrowserState::GetRequestContext() {
//  DCHECK_CURRENTLY_ON(web::WebThread::UI);
//  if (!request_context_getter_) {
//    ProtocolHandlerMap protocol_handlers;
//    protocol_handlers[kChromeUIScheme] =
//        web::URLDataManagerIOSBackend::CreateProtocolHandler(this);
//    request_context_getter_ =
//        base::WrapRefCounted(CreateRequestContext(&protocol_handlers));
//  }
//  return request_context_getter_.get();
//}

void ChromeBrowserState::UpdateCorsExemptHeader(
    network::mojom::NetworkContextParams* params) {
  variations::UpdateCorsExemptHeaderForVariations(params);
}
}
