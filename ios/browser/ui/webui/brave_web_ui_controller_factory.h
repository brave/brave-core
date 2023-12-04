// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_WEB_UI_CONTROLLER_FACTORY_H_
#define BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_WEB_UI_CONTROLLER_FACTORY_H_

#include <memory>

#import "ios/chrome/browser/ui/webui/chrome_web_ui_ios_controller_factory.h"
#import "ios/web/public/webui/web_ui_ios.h"
#import "ios/web/public/webui/web_ui_ios_controller.h"

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

class GURL;

class BraveWebUIControllerFactory : public ChromeWebUIIOSControllerFactory {
 public:
  std::unique_ptr<web::WebUIIOSController> CreateWebUIIOSControllerForURL(
      web::WebUIIOS* web_ui,
      const GURL& url) const override;

  NSInteger GetErrorCodeForWebUIURL(const GURL& url) const override;

  static BraveWebUIControllerFactory* GetInstance();

  BraveWebUIControllerFactory(const BraveWebUIControllerFactory&) = delete;
  BraveWebUIControllerFactory& operator=(const BraveWebUIControllerFactory&) =
      delete;

 protected:
  BraveWebUIControllerFactory();
  ~BraveWebUIControllerFactory() override;

 private:
  friend class base::NoDestructor<BraveWebUIControllerFactory>;
};

#endif  // BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_WEB_UI_CONTROLLER_FACTORY_H_
