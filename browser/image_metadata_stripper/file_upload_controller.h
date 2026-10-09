/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_IMAGE_METADATA_STRIPPER_FILE_UPLOAD_CONTROLLER_H_
#define BRAVE_BROWSER_IMAGE_METADATA_STRIPPER_FILE_UPLOAD_CONTROLLER_H_

#include <memory>

#include "brave/components/image_metadata_stripper/image_metadata_stripper.h"
#include "chrome/browser/ui/tabs/contents_observing_tab_feature.h"
#include "ui/base/unowned_user_data/scoped_unowned_user_data.h"

namespace content {
class WebContents;
}  // namespace content

namespace tabs {
class TabInterface;
}  // namespace tabs

namespace image_metadata_stripper {

// Name of the directory, directly under the user data directory, that holds
// the stripped copies of uploaded images. On macOS, for example:
//
//   ~/Library/Application Support/BraveSoftware/Brave-Browser/
//     ImageMetadataStripperTemp/
//       brave_image_stripXXXXXX/    <- one per tab
//         0/photo.jpg               <- one per stripped image
//         1/photo.jpg
//         2/screenshot.jpg
//         ...
//       brave_image_stripYYYYYY/
//         0/image.jpg
//         ...
//
// Each `FileUploadController` (one per tab) lazily creates its own uniquely
// named `kStripTempDirPrefix` directory on the first strip. Within it, every
// stripped image gets a numbered subdirectory (0, 1, 2, ...) so that the copy
// can keep the basename of its original without colliding with copies of
// identically named images. A tab's directory is deleted when the tab closes
// or its contents are discarded.
//
// TODO(https://github.com/brave/brave-browser/issues/58868): Whatever is left
// over, e.g. after a crash, should be removed at the next startup.
inline constexpr base::FilePath::CharType kStripperRootDirName[] =
    FILE_PATH_LITERAL("ImageMetadataStripperTemp");

// Prefix of the temporary root a `FileUploadController` creates under
// `kStripperRootDirName` for the copies it makes.
inline constexpr base::FilePath::CharType kStripTempDirPrefix[] =
    FILE_PATH_LITERAL("brave_image_strip");

// Makes the stripped copies of the images a tab uploads, and deletes them when
// the tab closes or its contents are discarded. Lives on the UI thread; all
// file work runs on a sequence of its own.
class FileUploadController : public tabs::ContentsObservingTabFeature {
 public:
  // For each source path, in order, the stripped copy or std::nullopt when no
  // copy was made.
  using StripCallback =
      base::OnceCallback<void(std::vector<std::optional<base::FilePath>>)>;

  DECLARE_USER_DATA(FileUploadController);

  ~FileUploadController() override;
  FileUploadController(const FileUploadController&) = delete;
  FileUploadController& operator=(const FileUploadController&) = delete;

  // The ownership of this controller is handled by BraveTabFeatures. Clients
  // must never assume ownership themselves.
  static FileUploadController* FromWebContents(
      content::WebContents* web_contents);

  // This will throw an error if |tab| already has a valid instance of
  // FileUploadController already present. This must never be called by any
  // other client than the owner of the FileUploadController which is
  // BraveTabFeatures.
  static std::unique_ptr<FileUploadController> MaybeCreate(
      tabs::TabInterface& tab);

  // Copies each of |srcs| that carries metadata to strip, and strips the copy.
  // |callback| runs on the calling sequence.
  // TODO(https://github.com/brave/brave-browser/issues/58868): Implement this
  // method.
  void Strip(std::vector<base::FilePath> srcs,
             StrippingClient client,
             StripCallback callback);

 private:
  explicit FileUploadController(tabs::TabInterface& tab);
  static FileUploadController* From(tabs::TabInterface* tab);

  // tabs::ContentsObservingTabFeature override.
  void OnDiscardContents(tabs::TabInterface* tab,
                         content::WebContents* old_contents,
                         content::WebContents* new_contents) override;

  // This helps to get the right FileUploadController instance for a given
  // WebContents.
  ui::ScopedUnownedUserData<FileUploadController> scoped_unowned_user_data_;
};
}  // namespace image_metadata_stripper

#endif  // BRAVE_BROWSER_IMAGE_METADATA_STRIPPER_FILE_UPLOAD_CONTROLLER_H_
