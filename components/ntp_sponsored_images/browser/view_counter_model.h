// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_BROWSER_VIEW_COUNTER_MODEL_H_
#define BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_BROWSER_VIEW_COUNTER_MODEL_H_

namespace ntp_sponsored_images {

class ViewCounterModel {
 public:
  ViewCounterModel();
  virtual ~ViewCounterModel();

  ViewCounterModel(const ViewCounterModel&) = delete;
  ViewCounterModel& operator=(const ViewCounterModel&) = delete;

  void set_total_image_count(int count) { total_image_count_ = count; }
  void ResetCurrentWallpaperImageIndex() { current_wallpaper_image_index_ = 0; }
  int current_wallpaper_image_index() const {
    return current_wallpaper_image_index_;
  }

  virtual bool ShouldShowWallpaper() const = 0;
  virtual void RegisterPageView() = 0;

 protected:
  int total_image_count_ = -1;
  int current_wallpaper_image_index_ = 0;
};

}  // namespace ntp_sponsored_images

#endif  // BRAVE_COMPONENTS_NTP_SPONSORED_IMAGES_BROWSER_VIEW_COUNTER_MODEL_H_
