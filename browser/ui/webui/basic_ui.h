/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BASIC_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BASIC_UI_H_

#include <memory>

#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "content/public/browser/web_ui_controller.h"

namespace content {
class RenderViewHost;
class WebUIDataSource;
class WebUI;
}

class Profile;

struct GzippedGritResourceMap;

content::WebUIDataSource* CreateBasicUIHTMLSource(
    Profile* profile,
    const std::string& name,
    const GzippedGritResourceMap* resource_map,
    size_t resouece_map_size,
    int html_resource_id);

class BasicUI : public content::WebUIController {
 public:
  BasicUI(content::WebUI* web_ui,
          const std::string& host,
          const GzippedGritResourceMap* resource_map,
          size_t resouece_map_size,
          int html_resource_id);
  ~BasicUI() override;

  // Called when subclass can set its webui properties.
  virtual void UpdateWebUIProperties() {}

 protected:
  bool IsSafeToSetWebUIProperties() const;
  content::RenderViewHost* GetRenderViewHost();

 private:
  class BasicUIWebContentsObserver;

  std::unique_ptr<BasicUIWebContentsObserver> observer_;

  DISALLOW_COPY_AND_ASSIGN(BasicUI);
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BASIC_UI_H_
