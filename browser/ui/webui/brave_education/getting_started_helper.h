/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_EDUCATION_GETTING_STARTED_HELPER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_EDUCATION_GETTING_STARTED_HELPER_H_

#include <list>
#include <memory>
#include <optional>
#include <string>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "url/gurl.h"

class Profile;

namespace network {
class SimpleURLLoader;
}

namespace brave_education {

class GettingStartedHelper {
 public:
  explicit GettingStartedHelper(Profile* profile);
  ~GettingStartedHelper();

  GettingStartedHelper(const GettingStartedHelper&) = delete;
  GettingStartedHelper& operator=(const GettingStartedHelper&) = delete;

  using GetEducationURLCallback = base::OnceCallback<void(std::optional<GURL>)>;
  void GetEducationURL(GetEducationURLCallback callback);

 private:
  void OnURLResponse(GURL webui_url, std::optional<std::string> body);
  void RunCallbacks(std::optional<GURL> webui_url);

  raw_ptr<Profile> profile_;
  std::unique_ptr<network::SimpleURLLoader> url_loader_;
  std::list<GetEducationURLCallback> url_callbacks_;
};

}  // namespace brave_education

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_EDUCATION_GETTING_STARTED_HELPER_H_
