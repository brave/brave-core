// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_DRAGDROP_OS_EXCHANGE_DATA_PROVIDER_ANDROID_H_
#define UI_BASE_DRAGDROP_OS_EXCHANGE_DATA_PROVIDER_ANDROID_H_

#include <map>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/pickle.h"
#include "ui/base/dragdrop/os_exchange_data.h"
#include "ui/gfx/geometry/vector2d.h"
#include "ui/gfx/image/image_skia.h"
#include "url/gurl.h"

namespace ui {

class ClipboardFormatType;

// OSExchangeData::Provider implementation for Android on linux.
class UI_BASE_EXPORT OSExchangeDataProviderAndroid
    : public OSExchangeData::Provider {
 public:
  OSExchangeDataProviderAndroid();
  ~OSExchangeDataProviderAndroid() override;

  // Overridden from OSExchangeData::Provider:
  std::unique_ptr<Provider> Clone() const override;
  void MarkOriginatedFromRenderer() override;
  bool DidOriginateFromRenderer() const override;
  void SetString(const base::string16& data) override;
  void SetURL(const GURL& url, const base::string16& title) override;
  void SetFilename(const base::FilePath& path) override;
  void SetFilenames(const std::vector<FileInfo>& filenames) override;
  void SetPickledData(const ClipboardFormatType& format,
                      const base::Pickle& data) override;
  bool GetString(base::string16* data) const override;
  bool GetURLAndTitle(OSExchangeData::FilenameToURLPolicy policy,
                      GURL* url,
                      base::string16* title) const override;
  bool GetFilename(base::FilePath* path) const override;
  bool GetFilenames(std::vector<FileInfo>* filenames) const override;
  bool GetPickledData(const ClipboardFormatType& format,
                      base::Pickle* data) const override;
  bool HasString() const override;
  bool HasURL(OSExchangeData::FilenameToURLPolicy policy) const override;
  bool HasFile() const override;
  bool HasCustomFormat(const ClipboardFormatType& format) const override;

  void SetHtml(const base::string16& html, const GURL& base_url) override;
  bool GetHtml(base::string16* html, GURL* base_url) const override;
  bool HasHtml() const override;
  void SetDragImage(const gfx::ImageSkia& image,
                    const gfx::Vector2d& cursor_offset) override;
  gfx::ImageSkia GetDragImage() const override;
  gfx::Vector2d GetDragImageOffset() const override;

 private:
  typedef std::map<ClipboardFormatType, base::Pickle> PickleData;

  // Returns true if |formats_| contains a file format and the file name can be
  // parsed as a URL.
  bool GetFileURL(GURL* url) const;

  // Returns true if |formats_| contains a string format and the string can be
  // parsed as a URL.
  bool GetPlainTextURL(GURL* url) const;

  // Actual formats that have been set. See comment above |known_formats_|
  // for details.
  int formats_;

  // String contents.
  base::string16 string_;

  // URL contents.
  GURL url_;
  base::string16 title_;

  // File name.
  std::vector<FileInfo> filenames_;

  // PICKLED_DATA contents.
  PickleData pickle_data_;

  // Drag image and offset data.
  gfx::ImageSkia drag_image_;
  gfx::Vector2d drag_image_offset_;

  // For HTML format
  base::string16 html_;
  GURL base_url_;

  DISALLOW_COPY_AND_ASSIGN(OSExchangeDataProviderAndroid);
};

}  // namespace ui

#endif  // UI_BASE_DRAGDROP_OS_EXCHANGE_DATA_PROVIDER_ANDROID_H_
