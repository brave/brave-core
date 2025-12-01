// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <ios/web/web_client.mm>

namespace web {

bool WebClient::ShouldBlockJavaScript(web::WebState* web_state,
                                      NSURLRequest* request) {
  return false;
}
NSString* WebClient::GetUserAgentForRequest(web::WebState* web_state,
                                            web::UserAgentType user_agent_type,
                                            NSURLRequest* request) {
  return nullptr;
}
bool WebClient::ShouldBlockUniversalLinks(web::WebState* web_state,
                                          NSURLRequest* request) {
  return false;
}
void WebClient::UpdateScripts() {}

}  // namespace web
