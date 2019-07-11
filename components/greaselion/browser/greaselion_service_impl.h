/* Copyright 2016 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_GREASELION_BROWSER_GREASELION_SERVICE_IMPL_H_
#define BRAVE_COMPONENTS_GREASELION_BROWSER_GREASELION_SERVICE_IMPL_H_

#include <map>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/greaselion/browser/greaselion_service.h"
#include "url/gurl.h"

namespace base {
class SequencedTaskRunner;
}

namespace extensions {
class Extension;
}

namespace greaselion {

class GreaselionDownloadService;

class GreaselionServiceImpl : public GreaselionService {
 public:
  explicit GreaselionServiceImpl(
      GreaselionDownloadService* download_service,
      const base::FilePath& install_directory,
      scoped_refptr<base::SequencedTaskRunner> task_runner);
  ~GreaselionServiceImpl() override;

  // GreaselionService overrides
  void SetFeatureEnabled(GreaselionFeature feature, bool enabled) override;

 private:
  void UpdateInstalledExtensions();
  void Install(scoped_refptr<extensions::Extension> extension);
  void PostInstall(const base::FilePath& extension_path);

  GreaselionDownloadService* download_service_;  // NOT OWNED
  GreaselionFeatures state_;
  const base::FilePath install_directory_;
  bool all_rules_installed_successfully_;
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  base::WeakPtrFactory<GreaselionServiceImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(GreaselionServiceImpl);
};

}  // namespace greaselion

#endif  // BRAVE_COMPONENTS_GREASELION_BROWSER_GREASELION_SERVICE_IMPL_H_
