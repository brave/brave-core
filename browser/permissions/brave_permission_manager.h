/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PERMISSION_BRAVE_PERMISSION_MANAGER_H_
#define BRAVE_BROWSER_PERMISSION_BRAVE_PERMISSION_MANAGER_H_

#include "chrome/browser/permissions/permission_manager.h"

class BravePermissionManager : public PermissionManager {
 public:
  explicit BravePermissionManager(Profile* profile);

 private:
  DISALLOW_COPY_AND_ASSIGN(BravePermissionManager);
};

#endif // BRAVE_BROWSER_PERMISSION_BRAVE_PERMISSION_MANAGER_H_
