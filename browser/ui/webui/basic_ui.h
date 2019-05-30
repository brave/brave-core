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
#include "brave/browser/version_info.h"

namespace content {
class RenderViewHost;
class WebUIDataSource;
class WebUI;
}

class Profile;

#if !defined(OS_ANDROID)
struct GzippedGritResourceMap;
#else
struct GritResourceMap;
#endif

#if !defined(OS_ANDROID)
content::WebUIDataSource* CreateBasicUIHTMLSource(
    Profile* profile,
    const std::string& name,
    const GritResourceMap* resource_map,
    size_t resouece_map_size,
    int html_resource_id);
#else
content::WebUIDataSource* CreateBasicUIHTMLSource(Profile* profile,
                                                  const std::string& name,
                                                  const GritResourceMap* resource_map,
                                                  size_t resouece_map_size,
                                                  int html_resource_id);
#endif

class BasicUI : public content::WebUIController {
 public:
#if !defined(OS_ANDROID)
  BasicUI(content::WebUI* web_ui,
          const std::string& host,
          const GritResourceMap* resource_map,
          size_t resouece_map_size,
          int html_resource_id);
#else
  BasicUI(content::WebUI* web_ui, const std::string& host,
      const GritResourceMap* resource_map, size_t resouece_map_size,
      int html_resource_id);
#endif
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
