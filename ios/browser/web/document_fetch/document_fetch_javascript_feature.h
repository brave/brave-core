// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_WEB_DOCUMENT_FETCH_DOCUMENT_FETCH_JAVASCRIPT_FEATURE_H_
#define BRAVE_IOS_BROWSER_WEB_DOCUMENT_FETCH_DOCUMENT_FETCH_JAVASCRIPT_FEATURE_H_

#include <map>
#include <optional>
#include <string>

#include "base/functional/callback_forward.h"
#include "base/no_destructor.h"
#include "ios/web/public/js_messaging/java_script_feature.h"

class GURL;

namespace web {
class WebState;
class WebStateID;
}  // namespace web

class DocumentFetchJavaScriptFeature : public web::JavaScriptFeature {
 public:
  // This feature holds per-request state; only a single static instance is
  // ever needed.
  static DocumentFetchJavaScriptFeature* GetInstance();

  using DownloadCallback =
      base::OnceCallback<void(int status_code, const std::string& base64_data)>;

  // Initiates a download of `url` in the context of `web_state`. `callback`
  // is invoked with the HTTP status code and base64-encoded response body when
  // the download completes, or with (0, "") on failure.
  void DownloadDocument(web::WebState* web_state,
                        const GURL& url,
                        DownloadCallback callback);

  // JavaScriptFeature:
  std::optional<std::string> GetScriptMessageHandlerName() const override;
  void ScriptMessageReceived(web::WebState* web_state,
                             const web::ScriptMessage& message) override;

 private:
  friend class base::NoDestructor<DocumentFetchJavaScriptFeature>;

  DocumentFetchJavaScriptFeature();
  ~DocumentFetchJavaScriptFeature() override;

  std::map<web::WebStateID, DownloadCallback> pending_callbacks_;
};

#endif  // BRAVE_IOS_BROWSER_WEB_DOCUMENT_FETCH_DOCUMENT_FETCH_JAVASCRIPT_FEATURE_H_
