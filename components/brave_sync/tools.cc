/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/tools.h"

#include <sstream>
#include <string>

#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "components/strings/grit/components_strings.h"
#include "crypto/random.h"
#include "crypto/sha2.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_sync {

namespace tools {

const char kOtherNodeOrder[] = "255.255.255";
const size_t kIdSize = 16;
const char kOtherBookmarksObjectIdSeed[] = "other_bookmarks_object_id";

namespace {
std::string PrintObjectId(uint8_t* bytes) {
  std::stringstream ss;
  for (size_t i = 0; i < kIdSize; ++i) {
    const uint8_t& byte = bytes[i];
    ss << std::dec << static_cast<int>(byte);
    if (i != kIdSize - 1) {
      ss << ", ";
    }
  }
  return ss.str();
}
}  // namespace

std::string GenerateObjectId() {
  // 16 random 8-bit unsigned numbers
  uint8_t bytes[kIdSize];
  crypto::RandBytes(bytes, sizeof(bytes));
  return PrintObjectId(bytes);
}

std::string GenerateObjectIdForOtherNode(const std::string old_id) {
  uint8_t bytes[kIdSize];
  const std::string input =
      old_id.empty() ? kOtherBookmarksObjectIdSeed : old_id;
  // Take first 16 bytes
  crypto::SHA256HashString(input, bytes, sizeof(bytes));
  return PrintObjectId(bytes);
}

std::string GetPlatformName() {
#if defined(OS_ANDROID)
  const std::string platform = "android";
#elif defined(OS_WIN)
  const std::string platform = "windows";
#elif defined(OS_LINUX)
  const std::string platform = "linux";
#elif defined(OS_MACOSX)
  const std::string platform = "macosx";
#elif defined(OS_IOS)
  const std::string platform = "ios";
#endif
  return platform;
}

bool IsTimeEmpty(const base::Time& time) {
  return time.is_null() || base::checked_cast<int64_t>(time.ToJsTime()) == 0;
}

// Get mutable node to prevent BookmarkMetaInfoChanged from being triggered
bookmarks::BookmarkNode* AsMutable(const bookmarks::BookmarkNode* node) {
  return const_cast<bookmarks::BookmarkNode*>(node);
}

std::string GetOtherNodeName() {
  return l10n_util::GetStringUTF8(IDS_BOOKMARK_BAR_OTHER_FOLDER_NAME);
}

}  // namespace tools

}  // namespace brave_sync
