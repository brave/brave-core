/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_MODEL_DATA_H_
#define BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_MODEL_DATA_H_

#include <memory>

class GURL;
class Profile;

namespace content {
class WebContents;
}  // namespace content

namespace sidebar {

class SidebarWebContentsDelegate;

// SidebarModelData represents sidebar each item's runtime state.
// Each sidebar item of built in type owns WebContents.
class SidebarModelData {
 public:
  explicit SidebarModelData(Profile* profile);
  virtual ~SidebarModelData();

  SidebarModelData(const SidebarModelData&) = delete;
  SidebarModelData& operator=(const SidebarModelData&) = delete;

  content::WebContents* GetWebContents();

  // nullptr if GetWebContents() is not called yet.
  content::WebContents* web_contents() const { return contents_.get(); }

  void LoadURL(const GURL& url);
  bool IsLoaded() const;
  bool need_favicon_update() const { return need_favicon_update_; }
  void set_need_favicon_update(bool need_update) {
    need_favicon_update_ = need_update;
  }

 private:
  Profile* profile_ = nullptr;
  bool need_favicon_update_ = false;
  std::unique_ptr<SidebarWebContentsDelegate> contents_delegate_;
  std::unique_ptr<content::WebContents> contents_;
};

}  // namespace sidebar

#endif  // BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_MODEL_DATA_H_
