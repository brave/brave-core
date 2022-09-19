/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NTP_BACKGROUND_CUSTOM_BACKGROUND_FILE_MANAGER_H_
#define BRAVE_BROWSER_NTP_BACKGROUND_CUSTOM_BACKGROUND_FILE_MANAGER_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/memory/ref_counted_memory.h"
#include "base/memory/weak_ptr.h"

namespace base {
class FilePath;
}

namespace gfx {
class Image;
}  // namespace gfx

namespace image_fetcher {
class ImageDecoder;
}  // namespace image_fetcher

class Profile;

// * Register new custom image file
//   * Decode and re-encode it and save it to our profile directory.
// * Manages custom images
//   * Have a list of custom images in Prefs.
//   * Make it sure that we have local files mapped to entries.
// * Deregisters custom image(TODO)
//   * remove the local file.
class CustomBackgroundFileManager final {
 public:
  using SaveFileCallback = base::OnceCallback<void(const base::FilePath&)>;

  explicit CustomBackgroundFileManager(Profile* profile);
  CustomBackgroundFileManager(const CustomBackgroundFileManager&) = delete;
  CustomBackgroundFileManager& operator=(const CustomBackgroundFileManager&) =
      delete;
  ~CustomBackgroundFileManager();

  void SaveImage(const base::FilePath& source_file_path,
                 SaveFileCallback callback);
  void MoveImage(const base::FilePath& source_file_path,
                 base::OnceCallback<void(bool /*result*/)> callback);

  base::FilePath GetCustomBackgroundDirectory() const;

 private:
  void MakeSureDirExists(
      base::OnceCallback<void(bool /*dir_exists*/)> on_dir_check);
  void ReadImage(const base::FilePath& path,
                 base::OnceCallback<void(const std::string&)> on_got_image);
  void SanitizeAndSaveImage(SaveFileCallback callback,
                            const base::FilePath& target_file_path,
                            const std::string& input);
  void DecodeImageInIsolatedProcess(
      const std::string& input,
      base::OnceCallback<void(const gfx::Image&)> on_decode);
  void SaveImageAsPNG(SaveFileCallback callback,
                      const base::FilePath& target_path,
                      const gfx::Image& image);

  raw_ptr<Profile> profile_ = nullptr;

  std::unique_ptr<image_fetcher::ImageDecoder> image_decoder_;

  base::WeakPtrFactory<CustomBackgroundFileManager> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_NTP_BACKGROUND_CUSTOM_BACKGROUND_FILE_MANAGER_H_
