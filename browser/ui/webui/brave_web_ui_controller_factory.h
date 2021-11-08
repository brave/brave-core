/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_WEB_UI_CONTROLLER_FACTORY_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_WEB_UI_CONTROLLER_FACTORY_H_

#include <memory>

#include "chrome/browser/ui/webui/chrome_web_ui_controller_factory.h"

namespace base {
class RefCountedMemory;
}

class BraveWebUIControllerFactory : public ChromeWebUIControllerFactory {
 public:
  BraveWebUIControllerFactory(const BraveWebUIControllerFactory&) = delete;
  BraveWebUIControllerFactory& operator=(const BraveWebUIControllerFactory&) =
      delete;

  content::WebUI::TypeID GetWebUIType(content::BrowserContext* browser_context,
                                      const GURL& url) override;
  std::unique_ptr<content::WebUIController> CreateWebUIControllerForURL(
      content::WebUI* web_ui,
      const GURL& url) override;

  static BraveWebUIControllerFactory* GetInstance();

 protected:
  friend struct base::DefaultSingletonTraits<BraveWebUIControllerFactory>;

  BraveWebUIControllerFactory();
  ~BraveWebUIControllerFactory() override;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_WEB_UI_CONTROLLER_FACTORY_H_
