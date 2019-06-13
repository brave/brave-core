/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_BRAVE_PROCESS_MANAGER_DELEGATE_H_
#define BRAVE_BROWSER_EXTENSIONS_BRAVE_PROCESS_MANAGER_DELEGATE_H_

#include "chrome/browser/extensions/chrome_process_manager_delegate.h"

namespace extensions {

class BraveProcessManagerDelegate : public ChromeProcessManagerDelegate {
  public:
    BraveProcessManagerDelegate();
    ~BraveProcessManagerDelegate() override;

  private:
    void OnProfileDestroyed(Profile* profile) override;

  DISALLOW_COPY_AND_ASSIGN(BraveProcessManagerDelegate);
};

}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_BRAVE_PROCESS_MANAGER_DELEGATE_H_
