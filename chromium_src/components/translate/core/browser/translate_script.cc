/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/translate/core/browser/translate_script.h"

#include "base/strings/strcat.h"
#include "brave/components/translate/core/browser/brave_translate_features.h"

#define TranslateScript ChromiumTranslateScript
#include "../../../../../../components/translate/core/browser/translate_script.cc"
#undef TranslateScript

namespace {

const char* kRedirectAllRequestsToSecurityOrigin = R"(
  const securityOriginHost = new URL(securityOrigin).host;
  const redirectToSecurityOrigin = (url) => {
    new_url = new URL(url);
    new_url.host = securityOriginHost;
    return new_url.toString();
  };
  if (typeof XMLHttpRequest.prototype.realOpen === 'undefined') {
    XMLHttpRequest.prototype.realOpen = XMLHttpRequest.prototype.open;
    XMLHttpRequest.prototype.open = function (method, url, async = true,
                                     user = "", password = "") {
      this.realOpen(method, redirectToSecurityOrigin(url), async, user,
                    password);
    }
  };
  originalOnLoadCSS = cr.googleTranslate.onLoadCSS;
  cr.googleTranslate.onLoadCSS = function (url) {
    originalOnLoadCSS(redirectToSecurityOrigin(url))
  };
  originalOnLoadJavascript = cr.googleTranslate.onLoadJavascript;
  cr.googleTranslate.onLoadJavascript = function (url) {
    originalOnLoadJavascript(redirectToSecurityOrigin(url))
  };
)";

}  // namespace
namespace translate {

void TranslateScript::OnScriptFetchComplete(bool success,
                                            const std::string& data) {
  if (!IsBraveTranslateGoAvailable()) {
    ChromiumTranslateScript::OnScriptFetchComplete(false, std::string());
    return;
  }

  const std::string new_data =
      base::StrCat({kRedirectAllRequestsToSecurityOrigin, data});
  ChromiumTranslateScript::OnScriptFetchComplete(success, new_data);
}

}  // namespace translate
