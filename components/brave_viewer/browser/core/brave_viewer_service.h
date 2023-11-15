// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_VIEWER_BROWSER_CORE_BRAVE_VIEWER_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_VIEWER_BROWSER_CORE_BRAVE_VIEWER_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/component_export.h"
#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/singleton.h"
#include "base/memory/weak_ptr.h"

class GURL;

namespace brave_viewer {

class COMPONENT_EXPORT(BRAVE_VIEWER_BROWSER_CORE) BraveViewerService {
 public:
  BraveViewerService(const BraveViewerService&) = delete;
  BraveViewerService& operator=(const BraveViewerService&) = delete;
  ~BraveViewerService();
  void GetTestScript(
      const GURL& url,
      base::OnceCallback<void(std::string test_script)> cb) const;
  static BraveViewerService* GetInstance();  // singleton
  void LoadNewComponentVersion(const base::FilePath& path);

 private:
  BraveViewerService();
  // Also called by BraveViewerTabHelperBrowserTest.
  void SetComponentPath(const base::FilePath& path);

  base::FilePath component_path_;

  base::WeakPtrFactory<BraveViewerService> weak_factory_{this};

  friend class BraveViewerTabHelperBrowserTest;  // Used for testing private
                                                 // methods.
  friend struct base::DefaultSingletonTraits<BraveViewerService>;
};

}  // namespace brave_viewer

#endif  // BRAVE_COMPONENTS_BRAVE_VIEWER_BROWSER_CORE_BRAVE_VIEWER_SERVICE_H_
