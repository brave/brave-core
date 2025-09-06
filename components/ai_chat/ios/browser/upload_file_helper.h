// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_IOS_BROWSER_UPLOAD_FILE_HELPER_H_
#define BRAVE_COMPONENTS_AI_CHAT_IOS_BROWSER_UPLOAD_FILE_HELPER_H_

#include <optional>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/callback.h"

namespace ai_chat {

class UploadFileHelperIOS {
 public:
  static void ProcessImageFileURL(
      NSURL* url,
      base::OnceCallback<void(std::optional<std::vector<uint8_t>>)> callback);

  static void ProcessImageData(
      const std::vector<uint8_t>& image_data,
      base::OnceCallback<void(std::optional<std::vector<uint8_t>>)> callback);
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_IOS_BROWSER_UPLOAD_FILE_HELPER_H_
