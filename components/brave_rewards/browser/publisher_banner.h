/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PUBLISHER_BANNER_
#define BRAVE_BROWSER_PUBLISHER_BANNER_

#include <string>
#include <vector>
#include <map>

namespace brave_rewards {
  struct PublisherBanner {
    PublisherBanner();
    ~PublisherBanner();
    PublisherBanner(const PublisherBanner& properties);

    std::string publisher_key;
    std::string title;
    std::string name;
    std::string description;
    std::string background;
    std::string logo;
    std::vector<double> amounts;
    std::string provider;
    std::map<std::string, std::string> links;
    bool verified;
  };

}  // namespace brave_rewards

#endif //BRAVE_BROWSER_PUBLISHER_BANNER_
