// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_PLAYER_CORE_BROWSER_BRAVE_PLAYER_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_PLAYER_CORE_BROWSER_BRAVE_PLAYER_SERVICE_H_

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

namespace brave_player {

class COMPONENT_EXPORT(BRAVE_PLAYER_CORE_BROWSER) BravePlayerService {
 public:
  static BravePlayerService* GetInstance();  // singleton
  BravePlayerService(const BravePlayerService&) = delete;
  BravePlayerService& operator=(const BravePlayerService&) = delete;
  ~BravePlayerService();

  void GetTestScript(
      const GURL& url,
      base::OnceCallback<void(std::string test_script)> cb) const;
  void LoadNewComponentVersion(const base::FilePath& path);

 private:
  friend class BravePlayerTabHelperBrowserTest;  // Used for testing private
                                                 // methods.
  friend struct base::DefaultSingletonTraits<BravePlayerService>;

  BravePlayerService();

  // Also called by BravePlayerTabHelperBrowserTest as well.
  void SetComponentPath(const base::FilePath& path);

  base::FilePath component_path_;

  base::WeakPtrFactory<BravePlayerService> weak_factory_{this};
};

}  // namespace brave_player

#endif  // BRAVE_COMPONENTS_BRAVE_PLAYER_CORE_BROWSER_BRAVE_PLAYER_SERVICE_H_
