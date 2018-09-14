/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_UTILITY_BRAVE_CONTENT_UTILITY_CLIENT_H_
#define BRAVE_UTILITY_BRAVE_CONTENT_UTILITY_CLIENT_H_

#include "chrome/utility/chrome_content_utility_client.h"

class BraveContentUtilityClient : public ChromeContentUtilityClient {
 public:
  BraveContentUtilityClient();
  ~BraveContentUtilityClient() override;

  void RegisterServices(StaticServiceMap* services) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BraveContentUtilityClient);
};

#endif  // BRAVE_UTILITY_BRAVE_CONTENT_UTILITY_CLIENT_H_
