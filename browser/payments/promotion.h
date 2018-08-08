/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PAYMENTS_PROMOTION_
#define BRAVE_BROWSER_PAYMENTS_PROMOTION_

#include <string>

namespace payments {
  struct Promotion {
    Promotion();
    ~Promotion();
    Promotion(const Promotion& properties);

    std::string promotionId;
    double amount;
  };

}  // namespace payments

#endif //BRAVE_BROWSER_PAYMENTS_PROMOTION_
