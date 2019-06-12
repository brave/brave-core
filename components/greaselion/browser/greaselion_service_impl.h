/* Copyright 2016 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_GREASELION_BROWSER_GREASELION_SERVICE_IMPL_H_
#define BRAVE_COMPONENTS_GREASELION_BROWSER_GREASELION_SERVICE_IMPL_H_

#include <map>
#include <string>
#include <vector>

#include "base/macros.h"
#include "brave/components/greaselion/browser/greaselion_service.h"
#include "url/gurl.h"

namespace greaselion {

class GreaselionDownloadService;

class GreaselionServiceImpl : public GreaselionService {
 public:
  explicit GreaselionServiceImpl(GreaselionDownloadService* download_service);
  ~GreaselionServiceImpl() override;

  // GreaselionService overrides
  bool ScriptsFor(const GURL& primary_url,
                  std::vector<std::string>* scripts) override;
  void SetFeatureEnabled(GreaselionFeature feature, bool enabled) override;

 private:
  GreaselionDownloadService* download_service_;  // NOT OWNED
  GreaselionFeatures state_;

  DISALLOW_COPY_AND_ASSIGN(GreaselionServiceImpl);
};

}  // namespace greaselion

#endif  // BRAVE_COMPONENTS_GREASELION_BROWSER_GREASELION_SERVICE_IMPL_H_
