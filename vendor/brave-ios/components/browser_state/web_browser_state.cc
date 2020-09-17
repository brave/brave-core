//
//  web_browser_state.cpp
//  base/third_party/double_conversion:double_conversion
//
//  Created by brandon on 2020-05-07.
//

#include "brave/vendor/brave-ios/components/browser_state/web_browser_state.h"

#include <memory>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/guid.h"
#include "base/location.h"
#include "base/memory/ref_counted.h"
#include "base/no_destructor.h"
#include "base/process/process_handle.h"
#include "base/task/post_task.h"
#include "base/token.h"

namespace web {
namespace {
const char kBrowserStateIdentifierKey[] = "BrowserStateIdentifierKey";
}  // namespace

BrowserState::BrowserState() {
  SetUserData(kBrowserStateIdentifierKey,
              std::make_unique<SupportsUserData::Data>());
}

BrowserState::~BrowserState() {
    
}

// static
BrowserState* BrowserState::FromSupportsUserData(
    base::SupportsUserData* supports_user_data) {
  if (!supports_user_data ||
      !supports_user_data->GetUserData(kBrowserStateIdentifierKey)) {
    return nullptr;
  }
  return static_cast<BrowserState*>(supports_user_data);
}

}  // namespace web
