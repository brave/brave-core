/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_web_ui_controller_factory.h"

using content::WebUI;
using content::WebUIController;


WebUI::TypeID BraveWebUIControllerFactory::GetWebUIType(
      content::BrowserContext* browser_context, const GURL& url) const {
  return ChromeWebUIControllerFactory::GetWebUIType(browser_context, url);
}

WebUIController* BraveWebUIControllerFactory::CreateWebUIControllerForURL(
    WebUI* web_ui,
    const GURL& url) const {
  return ChromeWebUIControllerFactory::CreateWebUIControllerForURL(
      web_ui, url);
}
