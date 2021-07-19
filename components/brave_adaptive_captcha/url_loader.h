/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADAPTIVE_CAPTCHA_URL_LOADER_H_
#define BRAVE_COMPONENTS_BRAVE_ADAPTIVE_CAPTCHA_URL_LOADER_H_

#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace brave_adaptive_captcha {

class UrlLoader {
 public:
  enum UrlMethod { GET = 0, PUT = 1, POST = 2 };

  struct UrlRequest {
    UrlRequest();
    ~UrlRequest();

    UrlRequest(const UrlRequest&) = delete;
    UrlRequest& operator=(UrlRequest&) = delete;

    std::string url;
    UrlMethod method = UrlMethod::GET;
    std::vector<std::string> headers;
    std::string content;
    std::string content_type;
    bool skip_log = false;
    uint32_t load_flags = 0;
  };

  struct UrlResponse {
    UrlResponse();
    ~UrlResponse();

    UrlResponse(const UrlResponse&) = delete;
    UrlResponse& operator=(UrlResponse&) = delete;

    std::string url;
    std::string error;
    int32_t status_code;
    std::string body;
    std::map<std::string, std::string> headers;
  };

  explicit UrlLoader(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~UrlLoader();

  UrlLoader(const UrlLoader&) = delete;
  UrlLoader& operator=(const UrlLoader&) = delete;

  using OnUrlRequestComplete =
      base::OnceCallback<void(const UrlLoader::UrlResponse&)>;
  void Load(const UrlRequest& url_request, OnUrlRequestComplete callback);

 private:
  using SimpleURLLoaderList =
      std::list<std::unique_ptr<network::SimpleURLLoader>>;

  void OnSimpleUrlLoaderComplete(SimpleURLLoaderList::iterator iter,
                                 OnUrlRequestComplete callback,
                                 std::unique_ptr<std::string> response_body);

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  SimpleURLLoaderList simple_url_loaders_;
};

}  // namespace brave_adaptive_captcha

#endif  // BRAVE_COMPONENTS_BRAVE_ADAPTIVE_CAPTCHA_URL_LOADER_H_
