// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// URLRequestJobFactory::CreateJob checks the protocol_handler_map_
// to see what requests can be handled
// The FactoryForMain contains the ProtocolHandlerMap
// This is initialized via ProfileIOSIOData::Init(ProtocolHandlerMap*
// protocol_handlers) Which is called via ProfileIOS::GetRequestContext

#include "ios/chrome/browser/shared/model/profile/profile_ios.h"

#include "ios/components/webui/web_ui_url_constants.h"
#include "ios/web/webui/url_data_manager_ios_backend.h"

// Add the chrome-untrusted scheme
auto CreateRequestContext_Brave(ProfileIOS* profile) {
  return [profile](ProtocolHandlerMap* protocol_handlers) {
    protocol_handlers->insert(
        {kChromeUIUntrustedScheme,
         web::URLDataManagerIOSBackend::CreateProtocolHandler(profile)});
    return profile->CreateRequestContext(protocol_handlers);
  };
}

#define CreateRequestContext CreateRequestContext_Brave(this)

#include "src/ios/chrome/browser/shared/model/profile/profile_ios.mm"

#undef CreateRequestContext
