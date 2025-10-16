/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/common/test_utils.h"

#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-shared.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "crypto/random.h"

namespace ai_chat {

std::vector<mojom::UploadedFilePtr> CreateSampleUploadedFiles(
    size_t number,
    mojom::UploadedFileType type) {
  std::vector<mojom::UploadedFilePtr> uploaded_files;
  for (size_t i = 0; i < number; ++i) {
    std::vector<uint8_t> file_data(base::RandGenerator(64));
    crypto::RandBytes(file_data);
    uploaded_files.emplace_back(
        mojom::UploadedFile::New("filename" + base::NumberToString(i),
                                 sizeof(file_data), file_data, type));
  }
  return uploaded_files;
}

}  // namespace ai_chat
