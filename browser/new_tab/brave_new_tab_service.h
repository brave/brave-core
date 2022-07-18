/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NEW_TAB_BRAVE_NEW_TAB_SERVICE_H_
#define BRAVE_BROWSER_NEW_TAB_BRAVE_NEW_TAB_SERVICE_H_

#include <memory>
#include <string>
#include <utility>

#include "base/memory/weak_ptr.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/web_contents.h"

class BraveNewTabService : public KeyedService {
 public:
  explicit BraveNewTabService(content::BrowserContext* browser_context);
  ~BraveNewTabService() override;

  std::unique_ptr<content::WebContents> GetNewTabContent();
  void PreloadNewTab();
  void Reset();

 private:
  std::unique_ptr<content::WebContents> cached_new_tab_;
  raw_ptr<content::BrowserContext> browser_context_;
};

#endif  // BRAVE_BROWSER_NEW_TAB_BRAVE_NEW_TAB_SERVICE_H_
