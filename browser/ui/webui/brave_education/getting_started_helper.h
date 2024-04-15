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
#include "brave/components/brave_education/common/education_content_urls.h"
#include "url/gurl.h"

class Profile;

namespace network {
class SimpleURLLoader;
}

namespace brave_education {

// A helper for determining the "getting started" WebUI URL for a given profile.
class GettingStartedHelper {
 public:
  explicit GettingStartedHelper(Profile* profile);
  ~GettingStartedHelper();

  GettingStartedHelper(const GettingStartedHelper&) = delete;
  GettingStartedHelper& operator=(const GettingStartedHelper&) = delete;

  using GetEducationURLCallback = base::OnceCallback<void(std::optional<GURL>)>;

  // Asynchronously returns a "getting started" education WebUI URL. Returns
  // `std::nullopt` if a "getting started" URL is not available (e.g. if
  // the network is not available or the web server is not returning a valid
  // response).
  void GetEducationURL(GetEducationURLCallback callback);

 private:
  void OnURLResponse(EducationContentType content_type,
                     std::optional<std::string> body);

  bool URLLoadedWithSuccess();

  void RunCallbacks(std::optional<GURL> webui_url);

  raw_ptr<Profile> profile_;
  std::unique_ptr<network::SimpleURLLoader> url_loader_;
  std::list<GetEducationURLCallback> url_callbacks_;
};

}  // namespace brave_education

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_EDUCATION_GETTING_STARTED_HELPER_H_
