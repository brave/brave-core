/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_UNTRUSTED_WEB_UI_CONTROLLER_FACTORY_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_UNTRUSTED_WEB_UI_CONTROLLER_FACTORY_H_

#include "ui/webui/untrusted_web_ui_controller_factory.h"

class BraveUntrustedWebUIControllerFactory
    : public ui::UntrustedWebUIControllerFactory {
 public:
  // Register the singleton instance of this class.
  static void RegisterInstance();

  BraveUntrustedWebUIControllerFactory();
  BraveUntrustedWebUIControllerFactory(
      const BraveUntrustedWebUIControllerFactory&) = delete;
  BraveUntrustedWebUIControllerFactory& operator=(
      const BraveUntrustedWebUIControllerFactory&) = delete;

 protected:
  const WebUIConfigMap& GetWebUIConfigMap() override;

 private:
  ~BraveUntrustedWebUIControllerFactory() override;
  WebUIConfigMap configs_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_UNTRUSTED_WEB_UI_CONTROLLER_FACTORY_H_
