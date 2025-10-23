// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_UI_WEBUI_FAVICON_SOURCE_H_
#define BRAVE_IOS_BROWSER_UI_WEBUI_FAVICON_SOURCE_H_

#include <map>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/task/cancelable_task_tracker.h"
#include "components/favicon/core/favicon_service.h"
#include "ios/web/public/webui/url_data_source_ios.h"

class ProfileIOS;

namespace base {
class RefCountedMemory;
}

namespace chrome {
enum class FaviconUrlFormat;
struct ParsedFaviconPath;
}  // namespace chrome

// FaviconSource is the gateway between network-level chrome:
// requests for favicons and the history backend that serves these.
// Two possible formats are allowed: chrome://favicon, kept only for backwards
// compatibility for extensions, and chrome://favicon2. Formats are described in
// favicon_url_parser.h.
class FaviconSource : public web::URLDataSourceIOS {
 public:
  // By default, favicons are served via a chrome trusted URL (chrome://). If
  // |serve_untrusted| is set to true, favicons will be served via
  // chrome-untrusted://. Note that chrome-untrusted:// only supports the
  // favicon2 URL format and does not support the legacy URL format.
  explicit FaviconSource(ProfileIOS* profile,
                         chrome::FaviconUrlFormat format,
                         bool serve_untrusted = false);

  FaviconSource(const FaviconSource&) = delete;
  FaviconSource& operator=(const FaviconSource&) = delete;

  ~FaviconSource() override;

  // web::URLDataSourceIOS implementation.
  std::string GetSource() const override;
  void StartDataRequest(
      std::string_view path,
      web::URLDataSourceIOS::GotDataCallback callback) override;
  std::string GetMimeType(std::string_view path) const override;
  bool AllowCaching() const override;
  bool ShouldReplaceExistingSource() const override;
  bool ShouldServiceRequest(const GURL& url) const override;

 protected:
  virtual bool UseDarkMode();
  virtual base::RefCountedMemory* LoadIconBytes(float scale_factor,
                                                int resource_id);

  raw_ptr<ProfileIOS> profile_;

 private:
  // Defines the allowed pixel sizes for requested favicons.
  enum IconSize { SIZE_16, SIZE_32, SIZE_64, NUM_SIZES };

  // Called when favicon data is available from the history backend. If
  // |bitmap_result| is valid, returns it to caller using |callback|. Otherwise
  // will send appropriate default icon for |size_in_dip| and |scale_factor|.
  void OnFaviconDataAvailable(
      web::URLDataSourceIOS::GotDataCallback callback,
      const chrome::ParsedFaviconPath& parsed,
      const favicon_base::FaviconRawBitmapResult& bitmap_result);

  // Sends the 16x16 DIP 1x default favicon.
  void SendDefaultResponse(web::URLDataSourceIOS::GotDataCallback callback,
                           bool force_light_mode = false);

  // Sends back default favicon or fallback monogram.
  void SendDefaultResponse(web::URLDataSourceIOS::GotDataCallback callback,
                           const chrome::ParsedFaviconPath& parsed);

  // Sends the default favicon.
  void SendDefaultResponse(web::URLDataSourceIOS::GotDataCallback callback,
                           int size_in_dip,
                           float scale_factor,
                           bool dark_mode);

  chrome::FaviconUrlFormat url_format_;

  base::CancelableTaskTracker cancelable_task_tracker_;

  bool serve_untrusted_;

  base::WeakPtrFactory<FaviconSource> weak_ptr_factory_{this};
};

#endif  // BRAVE_IOS_BROWSER_UI_WEBUI_FAVICON_SOURCE_H_
