/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_BASE_LOCAL_DATA_FILES_OBSERVER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_BASE_LOCAL_DATA_FILES_OBSERVER_H_

#include <string>

#include "base/files/file_path.h"

namespace brave_shields {

// The abstract base class for observers of the local data files service,
// which is the component that arbitrates access to various DAT files
// like tracking protection, video autoplay whitelist, etc.
class BaseLocalDataFilesObserver {
 public:
  BaseLocalDataFilesObserver();
  virtual ~BaseLocalDataFilesObserver();

  virtual void OnComponentReady(const std::string& component_id,
                                const base::FilePath& install_dir,
                                const std::string& manifest) = 0;
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_BASE_LOCAL_DATA_FILES_OBSERVER_H_
