/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_DEBOUNCE_BROWSER_DEBOUNCE_SERVICE_IMPL_H_
#define BRAVE_COMPONENTS_DEBOUNCE_BROWSER_DEBOUNCE_SERVICE_IMPL_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/debounce/browser/debounce_service.h"
#include "net/cookies/site_for_cookies.h"
#include "url/gurl.h"

namespace debounce {

class DebounceDownloadService;

class DebounceServiceImpl : public DebounceService {
 public:
  explicit DebounceServiceImpl(DebounceDownloadService* download_service);
  ~DebounceServiceImpl() override;

  // DebounceService overrides
  bool Debounce(const GURL& original_url, GURL* final_url) override;

 private:
  DebounceDownloadService* download_service_;  // NOT OWNED
  base::WeakPtrFactory<DebounceServiceImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(DebounceServiceImpl);
};

}  // namespace debounce

#endif  // BRAVE_COMPONENTS_DEBOUNCE_BROWSER_DEBOUNCE_SERVICE_IMPL_H_
