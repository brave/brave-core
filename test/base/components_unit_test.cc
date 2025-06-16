/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/test/base/components_unit_test.h"

#include "chrome/browser/ui/webui/chrome_web_ui_controller_factory.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/common/url_utils.h"

// components unit tests still run in chrome_unit_test_suite which registers
// ChromeWebUIControllerFactory::GetInstance()
class ComponentsUnitTest::StubWebUIWebUIControllerFactory
    : public content::WebUIControllerFactory {
 public:
  std::unique_ptr<content::WebUIController> CreateWebUIControllerForURL(
      content::WebUI* web_ui,
      const GURL& url) override {
    return nullptr;
  }
  content::WebUI::TypeID GetWebUIType(content::BrowserContext* browser_context,
                                      const GURL& url) override {
    return content::WebUI::kNoWebUI;
  }
  bool UseWebUIForURL(content::BrowserContext* browser_context,
                      const GURL& url) override {
    return content::HasWebUIScheme(url);
  }
};

ComponentsUnitTest::ComponentsUnitTest() {}
ComponentsUnitTest::~ComponentsUnitTest() {}

void ComponentsUnitTest::SetUp() {
  factory_ = std::make_unique<StubWebUIWebUIControllerFactory>();
  factory_registration_ =
      std::make_unique<content::ScopedWebUIControllerFactoryRegistration>(
          factory_.get(), ChromeWebUIControllerFactory::GetInstance());
}
