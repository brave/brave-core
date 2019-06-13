/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_GLOBAL_ERROR_BRAVE_GLOBAL_ERROR_SERVICE_H_
#define BRAVE_BROWSER_UI_GLOBAL_ERROR_BRAVE_GLOBAL_ERROR_SERVICE_H_

#include "chrome/browser/ui/global_error/global_error_service.h"

class BraveGlobalErrorService : public GlobalErrorService {
  public:
    BraveGlobalErrorService(Profile* profile);
    ~BraveGlobalErrorService() override;

    void NotifyErrorsChanged(GlobalError* error) override;

  private:
    Profile* profile_;

  DISALLOW_COPY_AND_ASSIGN(BraveGlobalErrorService);
};

#endif  // BRAVE_BROWSER_UI_GLOBAL_ERROR_BRAVE_GLOBAL_ERROR_SERVICE_H_
