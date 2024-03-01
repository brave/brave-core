/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/browser/mime_util.h"

#include <optional>
#include <utility>

#include "base/containers/fixed_flat_map.h"
#include "base/containers/flat_map.h"
#include "base/files/file_path.h"
#include "base/no_destructor.h"

namespace playlist {
namespace {

// References
// * List of mimetypes registered to IANA
//   * Video:
//   https://www.iana.org/assignments/media-types/media-types.xhtml#video
//   * Audio:
//   https://www.iana.org/assignments/media-types/media-types.xhtml#audio
// * Chromium media framework supports
//   * media/base/mime_util_internal.cc
// * Mimetype to extension
//   * https://developer.mozilla.org/en-US/docs/Web/HTTP/Basics_of_HTTP/MIME_types/Common_types
//
constexpr auto kMimeToExtensionMap =
    base::MakeFixedFlatMap<std::string_view, base::FilePath::StringPieceType>(
        {/*m3u8*/
         {"application/x-mpegurl", FILE_PATH_LITERAL("m3u8")},
         {"application/vnd.apple.mpegurl", FILE_PATH_LITERAL("m3u8")},
         {"audio/x-mpegurl", FILE_PATH_LITERAL("m3u8")},
         {"audio/mpegurl", FILE_PATH_LITERAL("m3u8")},
         /*aac*/
         {"audio/aac", FILE_PATH_LITERAL("aac")},
         /*flac*/
         {"audio/flac", FILE_PATH_LITERAL("flac")},
         /*mp3*/
         {"audio/mp3", FILE_PATH_LITERAL("mp3")},
         {"audio/x-mp3", FILE_PATH_LITERAL("mp3")},
         {"audio/mpeg", FILE_PATH_LITERAL("mp3")},
         /*wav*/
         {"audio/wav", FILE_PATH_LITERAL("wav")},
         {"audio/x-wav", FILE_PATH_LITERAL("wav")},
         /*webm*/
         {"audio/webm", FILE_PATH_LITERAL("weba")},
         {"video/webm", FILE_PATH_LITERAL("webm")},
         /*m4a*/
         {"audio/x-m4a", FILE_PATH_LITERAL("m4a")},
         /*3gp*/
         {"video/3gpp", FILE_PATH_LITERAL("3gp")},
         /*mp2t*/
         {"video/mp2t", FILE_PATH_LITERAL("ts")},
         /*mp4*/
         {"video/mp4", FILE_PATH_LITERAL("mp4")},
         {"audio/mp4", FILE_PATH_LITERAL("mp4")},
         /*mpeg*/
         {"video/mpeg", FILE_PATH_LITERAL("mpeg")},
         /*ogg*/
         {"application/ogg", FILE_PATH_LITERAL("ogx")},
         {"audio/ogg", FILE_PATH_LITERAL("oga")},
         {"video/ogg", FILE_PATH_LITERAL("ogv")},
         /*m4v*/
         {"video/x-m4v", FILE_PATH_LITERAL("m4v")}});

}  // namespace

namespace mime_util {

std::optional<base::FilePath::StringType> GetFileExtensionForMimetype(
    std::string_view mime_type) {
  if (decltype(kMimeToExtensionMap)::const_iterator iter =
          kMimeToExtensionMap.find(mime_type);
      iter != kMimeToExtensionMap.end()) {
    return base::FilePath::StringType(iter->second);
  }

  return std::nullopt;
}

std::optional<std::string> GetMimeTypeForFileExtension(
    base::FilePath::StringPieceType file_extension) {
  static const base::NoDestructor<
      base::flat_map<base::FilePath::StringPieceType, std::string_view>>
      kExtensionToMimeMap(([]() {
        base::flat_map<base::FilePath::StringPieceType, std::string_view> map;
        for (const auto& [mime, ext] : kMimeToExtensionMap) {
          map[ext] = mime;
        }
        return map;
      })());

  if (auto iter = kExtensionToMimeMap->find(file_extension);
      iter != kExtensionToMimeMap->end()) {
    return std::string(iter->second);
  }

  return std::nullopt;
}

}  // namespace mime_util
}  // namespace playlist
