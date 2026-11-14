/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/installer/snap_tar_utils.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "base/check.h"
#include "base/containers/span.h"
#include "base/containers/span_reader.h"
#include "base/numerics/checked_math.h"
#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "base/strings/string_view_util.h"

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

// Parses an octal ASCII string (null- or space-terminated). The 12-byte ustar
// size field holds up to 36 bits, which does not fit a 32-bit size_t, so the
// value is accumulated into a checked uint64_t and the caller narrows it only
// after bounds-checking against the archive length.
std::optional<uint64_t> ParseOctal(base::span<const char> field) {
  base::CheckedNumeric<uint64_t> value = 0;
  for (char c : field) {
    if (c == '\0' || c == ' ') {
      break;
    }
    if (c < '0' || c > '7') {
      return std::nullopt;
    }
    value = value * 8 + static_cast<uint64_t>(c - '0');
  }
  if (!value.IsValid()) {
    return std::nullopt;
  }
  return value.ValueOrDie();
}

// Returns the NUL-terminated string within a fixed-width header field.
std::string_view FieldValue(base::span<const char> field) {
  std::string_view value = base::as_string_view(field);
  return value.substr(0, value.find('\0'));
}

// Reconstructs the full entry path from the ustar header. |header| is always a
// full block, so these field subspans are in bounds by construction.
std::string GetEntryPath(base::span<const char, kBlockSize> header) {
  const std::string_view name =
      FieldValue(header.subspan<kNameOffset, kNameSize>());
  const std::string_view prefix =
      FieldValue(header.subspan<kPrefixOffset, kPrefixSize>());

  if (!prefix.empty()) {
    return base::StrCat({prefix, "/", name});
  }
  return std::string(name);
}

// |path| is the entry path after the archive root component is stripped.
// Rejects an empty path, a leading '/', and any ".." component. Entry names
// are attacker controlled and select which member is read; they are not used
// as output paths.
//
// Deliberately not routed through base::FilePath: tar member names are always
// '/'-separated, while FilePath additionally treats '\' as a separator on
// Windows (FilePath::kSeparators) and carries drive-letter semantics, so the
// same archive would be parsed differently per platform. FilePath is used only
// for the paths we write under the temp dir.
bool IsSafeRelativePath(std::string_view path) {
  if (path.empty() || path.front() == '/') {
    return false;
  }
  for (std::string_view component : base::SplitStringPiece(
           path, "/", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL)) {
    if (component == "..") {
      return false;
    }
  }
  return true;
}

// Strips the archive's root directory component (e.g. "package/") so entries
// can be matched against the relative paths used in snap.manifest.json.
// Returns nullopt for entries that are not inside a root directory, or whose
// remainder is unsafe.
std::optional<std::string_view> ToSnapRelativePath(
    std::string_view entry_path) {
  const size_t slash = entry_path.find('/');
  if (slash == std::string_view::npos) {
    return std::nullopt;
  }
  const std::string_view relative = entry_path.substr(slash + 1);
  if (!IsSafeRelativePath(relative)) {
    return std::nullopt;
  }
  return relative;
}

// Core iterator: calls |visitor| for each regular file entry.
// |visitor| receives (relative_path, file_span) and returns true to stop
// early. |relative_path| has the archive's root directory component
// stripped.
// Entry sizes come from attacker-controlled headers, so every length here goes
// through base::CheckedNumeric and every read through base::SpanReader, which
// bounds-checks against what is left of the archive instead of relying on
// offset arithmetic that could wrap.
template <typename Visitor>
bool IterateTar(base::span<const char> data, Visitor visitor) {
  base::SpanReader<const char> reader(data);
  while (auto header_block = reader.Read<kBlockSize>()) {
    const base::span<const char, kBlockSize> header = *header_block;

    if (header[kNameOffset] == '\0') {
      break;  // End-of-archive marker.
    }

    std::optional<uint64_t> parsed_size =
        ParseOctal(header.subspan<kSizeOffset, kSizeLength>());
    if (!parsed_size) {
      return false;  // Malformed archive.
    }

    // Narrow to size_t only now: a size that cannot be represented is a
    // malformed archive rather than a value to silently truncate. This is the
    // case that wraps on 32-bit builds, where size_t is narrower than the
    // 36 bits the size field can express.
    size_t file_size = 0;
    if (!base::CheckedNumeric<uint64_t>(*parsed_size)
             .AssignIfValid(&file_size)) {
      return false;
    }

    // Read the content, then skip the padding to reach the next header. The
    // reader rejects a declared size that runs past the end of the archive.
    std::optional<base::span<const char>> file_span = reader.Read(file_size);
    if (!file_span) {
      return false;  // Truncated archive.
    }
    // Entry contents are padded up to the next block boundary.
    size_t padding = 0;
    if (!base::CheckMod(base::CheckSub(kBlockSize,
                                       base::CheckMod(file_size, kBlockSize)),
                        kBlockSize)
             .AssignIfValid(&padding)) {
      return false;
    }
    if (!reader.Skip(padding)) {
      return false;  // Truncated archive.
    }

    const char type = header[kTypeOffset];
    const bool is_regular = (type == '0' || type == '\0' || type == '7');

    if (is_regular && file_size > 0) {
      const std::string entry_path = GetEntryPath(header);
      std::optional<std::string_view> relative_path =
          ToSnapRelativePath(entry_path);
      if (relative_path && visitor(*relative_path, *file_span)) {
        return true;  // Early exit requested by visitor.
      }
    }
  }
  return true;
}

}  // namespace

std::optional<std::string> ExtractFileFromTar(std::string_view tar_data,
                                              std::string_view relative_path) {
  auto data = base::as_chars(base::as_byte_span(tar_data));
  std::optional<std::string> result;

  bool ok = IterateTar(
      data, [&](std::string_view path, base::span<const char> file_span) {
        if (path == relative_path) {
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

std::optional<SnapTarResult> ExtractSnapFiles(
    std::string_view tar_data,
    std::string_view bundle_file_path) {
  // The bundle path comes from the manifest and must be known; guessing which
  // file is the bundle would let a crafted archive choose it for us.
  CHECK(!bundle_file_path.empty());

  auto data = base::as_chars(base::as_byte_span(tar_data));
  SnapTarResult result;
  bool found_manifest = false;
  bool found_bundle = false;

  bool ok = IterateTar(
      data, [&](std::string_view path, base::span<const char> file_span) {
        if (!found_manifest && path == "snap.manifest.json") {
          result.manifest_json.assign(file_span.begin(), file_span.end());
          found_manifest = true;
        } else if (!found_bundle && path == bundle_file_path) {
          result.bundle_js.assign(file_span.begin(), file_span.end());
          found_bundle = true;
        }
        return found_manifest && found_bundle;  // Stop when both found.
      });

  if (!ok || !found_manifest || !found_bundle) {
    return std::nullopt;
  }
  return result;
}

}  // namespace brave_wallet
