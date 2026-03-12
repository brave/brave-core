/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/snap_tar_utils.h"

#include <string_view>

#include "base/containers/span.h"

namespace brave_wallet {

namespace {

// POSIX ustar header layout constants.
constexpr size_t kBlockSize = 512;
constexpr size_t kNameOffset = 0;
constexpr size_t kNameSize = 100;
constexpr size_t kSizeOffset = 124;
constexpr size_t kSizeLength = 12;
constexpr size_t kTypeOffset = 156;
constexpr size_t kPrefixOffset = 345;
constexpr size_t kPrefixSize = 155;

// Parses an octal ASCII string (null- or space-terminated) into a size_t.
bool ParseOctal(base::span<const char> field, size_t* out) {
  *out = 0;
  for (char c : field) {
    if (c == '\0' || c == ' ') {
      break;
    }
    if (c < '0' || c > '7') {
      return false;
    }
    *out = (*out << 3) | static_cast<size_t>(c - '0');
  }
  return true;
}

// Returns the length of the null-terminated string within |field|.
size_t FieldLength(base::span<const char> field) {
  for (size_t i = 0; i < field.size(); ++i) {
    if (field[i] == '\0') {
      return i;
    }
  }
  return field.size();
}

// Reconstructs the full entry path from the ustar header.
std::string GetEntryPath(base::span<const char> header) {
  auto name_field = header.subspan(kNameOffset, kNameSize);
  auto prefix_field = header.subspan(kPrefixOffset, kPrefixSize);

  auto name_data = name_field.first(FieldLength(name_field));
  auto prefix_data = prefix_field.first(FieldLength(prefix_field));

  std::string name(name_data.begin(), name_data.end());
  std::string prefix(prefix_data.begin(), prefix_data.end());

  if (!prefix.empty()) {
    return prefix + "/" + name;
  }
  return name;
}

bool EndsWithSuffix(std::string_view path, std::string_view suffix) {
  return path.size() >= suffix.size() &&
         path.substr(path.size() - suffix.size()) == suffix;
}

bool IsDistJsFile(std::string_view path) {
  return path.find("/dist/") != std::string_view::npos &&
         EndsWithSuffix(path, ".js");
}

// Core iterator: calls |visitor| for each regular file entry.
// |visitor| receives (path, file_span) and returns true to stop early.
template <typename Visitor>
bool IterateTar(base::span<const char> data, Visitor visitor) {
  size_t offset = 0;
  while (offset + kBlockSize <= data.size()) {
    auto header = data.subspan(offset, kBlockSize);
    offset += kBlockSize;

    if (header[kNameOffset] == '\0') {
      break;  // End-of-archive marker.
    }

    size_t file_size = 0;
    if (!ParseOctal(header.subspan(kSizeOffset, kSizeLength), &file_size)) {
      return false;  // Malformed archive.
    }

    char type = header[kTypeOffset];
    bool is_regular = (type == '0' || type == '\0' || type == '7');

    if (is_regular && file_size > 0) {
      if (offset + file_size > data.size()) {
        return false;  // Truncated archive.
      }
      std::string path = GetEntryPath(header);
      auto file_span = data.subspan(offset, file_size);
      if (visitor(path, file_span)) {
        return true;  // Early exit requested by visitor.
      }
    }

    size_t padded = ((file_size + kBlockSize - 1) / kBlockSize) * kBlockSize;
    offset += padded;
  }
  return true;
}

}  // namespace

std::optional<std::string> ExtractFileFromTar(const std::string& tar_data,
                                               std::string_view path_suffix) {
  auto data = base::as_chars(base::as_byte_span(tar_data));
  std::optional<std::string> result;

  bool ok = IterateTar(data, [&](const std::string& path,
                                  base::span<const char> file_span) {
    if (EndsWithSuffix(path, path_suffix)) {
      result.emplace(file_span.begin(), file_span.end());
      return true;  // Stop.
    }
    return false;
  });

  if (!ok) {
    return std::nullopt;  // Malformed archive.
  }
  return result;
}

std::optional<SnapTarResult> ExtractSnapFiles(const std::string& tar_data,
                                              std::string_view bundle_file_path) {
  auto data = base::as_chars(base::as_byte_span(tar_data));
  SnapTarResult result;
  bool found_manifest = false;
  bool found_bundle = false;

  bool ok = IterateTar(data, [&](const std::string& path,
                                  base::span<const char> file_span) {
    if (!found_manifest && EndsWithSuffix(path, "snap.manifest.json")) {
      result.manifest_json.assign(file_span.begin(), file_span.end());
      found_manifest = true;
    } else if (!found_bundle) {
      bool matches = !bundle_file_path.empty()
                         ? EndsWithSuffix(path, bundle_file_path)
                         : IsDistJsFile(path);
      if (matches) {
        result.bundle_js.assign(file_span.begin(), file_span.end());
        found_bundle = true;
      }
    }
    return found_manifest && found_bundle;  // Stop when both found.
  });

  if (!ok || !found_manifest || !found_bundle) {
    return std::nullopt;
  }
  return result;
}

}  // namespace brave_wallet
