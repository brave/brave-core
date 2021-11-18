/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_SHELL_INTEGRATION_H_
#define BRAVE_BROWSER_BRAVE_SHELL_INTEGRATION_H_

#include "chrome/browser/shell_integration.h"

namespace shell_integration {

class BraveDefaultBrowserWorker : public DefaultBrowserWorker {
 public:
  BraveDefaultBrowserWorker();

  BraveDefaultBrowserWorker(const BraveDefaultBrowserWorker&) = delete;
  BraveDefaultBrowserWorker& operator=(const BraveDefaultBrowserWorker&) =
      delete;

 protected:
  ~BraveDefaultBrowserWorker() override;

  void SetAsDefaultImpl(base::OnceClosure on_finished_callback) override;
};

}  // namespace shell_integration

#endif  // BRAVE_BROWSER_BRAVE_SHELL_INTEGRATION_H_
