/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NTP_BACKGROUND_CUSTOM_BACKGROUND_FILE_MANAGER_H_
#define BRAVE_BROWSER_NTP_BACKGROUND_CUSTOM_BACKGROUND_FILE_MANAGER_H_

#include <memory>
#include <string>
#include <type_traits>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/memory/ref_counted_memory.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/ntp_background_images/browser/url_constants.h"
#include "url/gurl.h"
#include "url/url_util.h"

#if defined(OS_WIN)
#include "base/strings/sys_string_conversions.h"
#endif

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
// * Deregisters custom image
//   * remove the local file.
class CustomBackgroundFileManager final {
 public:
  // |Converter| is a convience class to convert values to and from each layer.
  //
  //    [Web UI] - GURL with percent encoded path.
  //       ||      The path value should be same with the value in PrefService
  //     (path)
  //       ||
  //     (value)
  //       ||
  //  [PrefService] - std::string. The value is a file name. Encoding is
  //       ||         usually UTF8, but on some platforms it might not be
  //     (value)      specified.
  //       ||
  //   (file name)
  //       ||
  //  [File System] - base::FilePath. On Windows, it uses wide string.
  //                  and on other platforms, uses string but encoding might
  //                  not be specified.
  //
  // This class is designed to be used for a short time and then destroyed.
  // Don't pass this class around code base. Use this just right where you
  // need to convert values.
  //
  // example:
  //  auto file_path = Converter(url, file_manager).To<base::FilePath>();
  //  auto url = Converter(prefs_value).To<GURL>();
  //
  template <class FromT>
  class Converter final {
   public:
    explicit Converter(const FromT& value,
                       CustomBackgroundFileManager* file_manager = nullptr)
        : file_manager_(file_manager) {
      if constexpr (std::is_same_v<FromT, std::string>) {
        DCHECK(!value.starts_with(ntp_background_images::kCustomWallpaperURL))
            << "URLs should be passed in as a GURL";
        value_ = value;
      } else if constexpr (std::is_same_v<FromT, GURL>) {
        // GURL(webui data url) -> std::string(prefs value)
        // When GURL is given, its path is percent encoded pref name. So
        // decode it and create Converter with it.
        DCHECK(value.SchemeIs("chrome") &&
               value.host() == ntp_background_images::kCustomWallpaperHost)
            << "Not a custom wallpaper URL";

        // remove leading slash
        const auto path = value.path().substr(1);
        DCHECK(!path.empty()) << "URL path is empty " << value;
        url::RawCanonOutputT<char16_t> decoded_value;
        url::DecodeURLEscapeSequences(
            path, url::DecodeURLMode::kUTF8OrIsomorphic, &decoded_value);
        value_ = base::UTF16ToUTF8(
            std::u16string(decoded_value.data(), decoded_value.length()));
      } else {
        // FilePath(local file path) -> std::string(prefs value)
        static_assert(std::is_same_v<FromT, base::FilePath>,
                      "FromT must be one of std::string, GURL, base::FilePath");

        // When base::FilePath is given, its file name is used as pref value.
        // But the file path's underlying type and encoding is platform
        // dependent. So, extract file name and convert it to UTF8 if needed.
#if defined(OS_WIN)
        auto file_name = base::SysWideToUTF8(value.BaseName().value());
#else
        auto file_name = std::string(value.BaseName().value().c_str());
#endif
        DCHECK(!file_name.empty())
            << "Couldn't extract file name from the given path " << value;
        value_ = file_name;
      }
    }
    Converter(const Converter&) = delete;
    Converter& operator=(const Converter&) = delete;
    Converter(Converter&&) noexcept = delete;
    Converter& operator=(Converter&&) noexcept = delete;
    ~Converter() = default;

    // Converter functions. Not allowing to convert to what it was created from.
    template <class ToT>
    [[nodiscard]] std::enable_if_t<!std::is_same_v<FromT, ToT>, ToT> To()
        const&& {
      if constexpr (std::is_same_v<ToT, std::string>) {
        return value_;
      } else if constexpr (std::is_same_v<ToT, GURL>) {
        // std::string(pref_value) -> GURL(webui data url)
        // Do percent encoding and compose it with base url so that it can
        // be used as webui data url.
        url::RawCanonOutputT<char> encoded;
        url::EncodeURIComponent(value_, &encoded);
        return GURL(ntp_background_images::kCustomWallpaperURL +
                    std::string(encoded.data(), encoded.length()));
      } else {
        static_assert(std::is_same_v<ToT, base::FilePath>,
                      "ToT must be one of std::string, GURL, base::FilePath");
        // std::string(pref_value) -> base::FilePath(local file path)
        DCHECK(file_manager_) << "Converting to local file path requires "
                                 "CustomBackgroundFileManager";
        base::FilePath file_path =
            file_manager_->GetCustomBackgroundDirectory();
#if defined(OS_WIN)
        file_path = file_path.Append(base::SysUTF8ToWide(value_));
#else
        file_path = file_path.Append(value_);
#endif
        return file_path;
      }
    }

   private:
    raw_ptr<CustomBackgroundFileManager> file_manager_ = nullptr;
    std::string value_;
  };

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
  void RemoveImage(const base::FilePath& file_path,
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
