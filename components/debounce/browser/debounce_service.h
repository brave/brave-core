/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_DEBOUNCE_BROWSER_DEBOUNCE_SERVICE_H_
#define BRAVE_COMPONENTS_DEBOUNCE_BROWSER_DEBOUNCE_SERVICE_H_

#include "base/macros.h"
#include "components/keyed_service/core/keyed_service.h"
#include "url/gurl.h"

namespace debounce {

class DebounceService : public KeyedService {
 public:
  DebounceService() = default;
  virtual bool Debounce(const GURL& original_url, GURL* final_url) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(DebounceService);
};

}  // namespace debounce

#endif  // BRAVE_COMPONENTS_DEBOUNCE_BROWSER_DEBOUNCE_SERVICE_H_
