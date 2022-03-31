/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/translate/core/browser/translate_script.h"

#include "base/callback.h"
#include "base/strings/strcat.h"
#include "base/strings/stringprintf.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "brave/components/translate/core/common/brave_translate_constants.h"
#include "brave/components/translate/core/common/brave_translate_features.h"

#define TranslateScript ChromiumTranslateScript
#include "src/components/translate/core/browser/translate_script.cc"
#undef TranslateScript

namespace {

const char* kRedirectAllRequestsToSecurityOrigin = R"(
  const useGoogleTranslateEndpoint = %s;
  const securityOriginHost = new URL(securityOrigin).host;
  const redirectToSecurityOrigin = (url) => {
    new_url = new URL(url);
    if (useGoogleTranslateEndpoint && new_url.pathname === '/translate') {
      new_url.host = 'translate.googleapis.com';
      new_url.pathname = '/translate_a/t';
      return new_url.toString();
    }
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

// Redirect the translate script request to the Brave endpoints.
GURL ChromiumTranslateScript::AddHostLocaleToUrl(const GURL& url) {
  GURL result = ::translate::AddHostLocaleToUrl(url);
  const GURL google_translate_script(kScriptURL);
  if (result.host_piece() == google_translate_script.host_piece()) {
    const GURL brave_translate_script(kBraveTranslateScriptURL);
    GURL::Replacements replaces;
    replaces.SetHostStr(brave_translate_script.host_piece());
    replaces.SetPathStr(brave_translate_script.path_piece());
    return result.ReplaceComponents(replaces);
  }
  return result;
}

void TranslateScript::Request(RequestCallback callback, bool is_incognito) {
  if (!IsBraveTranslateGoAvailable()) {
    base::SequencedTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), false));
    return;
  }
  ChromiumTranslateScript::Request(std::move(callback), is_incognito);
}

void TranslateScript::OnScriptFetchComplete(bool success,
                                            const std::string& data) {
  const std::string new_data = base::StrCat(
      {base::StringPrintf(
           kRedirectAllRequestsToSecurityOrigin,
           translate::UseGoogleTranslateEndpoint() ? "true" : "false"),
       data});
  ChromiumTranslateScript::OnScriptFetchComplete(success, new_data);
}

}  // namespace translate
