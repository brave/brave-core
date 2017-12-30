// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_UI_WEBUI_CHROME_WEB_UI_CONTROLLER_FACTORY_H_
#define BRAVE_BROWSER_UI_WEBUI_CHROME_WEB_UI_CONTROLLER_FACTORY_H_

#include "chrome/browser/ui/webui/chrome_web_ui_controller_factory.h"

class Profile;

namespace base {
class RefCountedMemory;
}

class BraveWebUIControllerFactory : public ChromeWebUIControllerFactory {
 public:
  content::WebUI::TypeID GetWebUIType(content::BrowserContext* browser_context,
                                      const GURL& url) const override;
  content::WebUIController* CreateWebUIControllerForURL(
      content::WebUI* web_ui,
      const GURL& url) const override;

  static BraveWebUIControllerFactory* GetInstance();

 protected:
  friend struct base::DefaultSingletonTraits<BraveWebUIControllerFactory>;

  BraveWebUIControllerFactory();
  ~BraveWebUIControllerFactory() override;

 private:
  friend struct base::DefaultSingletonTraits<BraveWebUIControllerFactory>;
  DISALLOW_COPY_AND_ASSIGN(BraveWebUIControllerFactory);
};

#endif  // BRAVE_BROWSER_UI_WEBUI_CHROME_WEB_UI_CONTROLLER_FACTORY_H_
