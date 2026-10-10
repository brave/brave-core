/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_IMAGE_METADATA_STRIPPER_FILE_UPLOAD_CONTROLLER_H_
#define BRAVE_BROWSER_IMAGE_METADATA_STRIPPER_FILE_UPLOAD_CONTROLLER_H_

#include <memory>

#include "chrome/browser/ui/tabs/contents_observing_tab_feature.h"

namespace content {
class WebContents;
}  // namespace content

namespace tabs {
class TabInterface;
}  // namespace tabs

namespace image_metadata_stripper {

// Makes the stripped copies of the images a tab uploads, and deletes them when
// the tab closes or its contents are discarded. Lives on the UI thread; all
// file work runs on a sequence of its own.
class FileUploadController : public tabs::ContentsObservingTabFeature {
 public:
  ~FileUploadController() override;
  FileUploadController(const FileUploadController&) = delete;
  FileUploadController& operator=(const FileUploadController&) = delete;

  // This will throw an error if |tab| already has a valid instance of
  // FileUploadController already present. This must never be called by any
  // other client than the owner of the FileUploadController which is
  // BraveTabFeatures.
  static std::unique_ptr<FileUploadController> MaybeCreate(
      tabs::TabInterface& tab);

 private:
  explicit FileUploadController(tabs::TabInterface& tab);

  // tabs::ContentsObservingTabFeature override.
  void OnDiscardContents(tabs::TabInterface* tab,
                         content::WebContents* old_contents,
                         content::WebContents* new_contents) override;
};
}  // namespace image_metadata_stripper

#endif  // BRAVE_BROWSER_IMAGE_METADATA_STRIPPER_FILE_UPLOAD_CONTROLLER_H_
