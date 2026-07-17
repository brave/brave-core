// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/workspace/workspace_file_ops.h"

#include <algorithm>
#include <array>
#include <string_view>
#include <vector>

#include "base/containers/span.h"
#include "base/files/file.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/pattern.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "build/build_config.h"
#include "third_party/re2/src/re2/re2.h"

namespace ai_chat::workspace {

namespace {

// An error, addressed to the model, explaining that a path fell outside the
// workspace and naming the root so it can retry with a correct path. The model
// isn't otherwise told the root, so surfacing it here lets it self-correct.
std::string OutsideRootError(const base::FilePath& canonical_root,
                             const std::string& relative) {
  return base::StrCat(
      {"'", relative,
       "' is not inside the workspace folder. The workspace root is '",
       canonical_root.AsUTF8Unsafe(),
       "'. Pass a path relative to that root (for example \"src/app.ts\"), or "
       "omit the path to use the whole workspace."});
}

// The workspace-relative, always-'/'-separated form of `full` under `root`.
// `root` must be a parent of (or equal to) `full`.
std::string RelativePosixPath(const base::FilePath& root,
                              const base::FilePath& full) {
  base::FilePath rel;
  if (full == root || !root.AppendRelativePath(full, &rel)) {
    return std::string();
  }
  std::string out = rel.AsUTF8Unsafe();
#if BUILDFLAG(IS_WIN)
  std::replace(out.begin(), out.end(), '\\', '/');
#endif
  return out;
}

// Reads at most kMaxReadBytes of `path`. Sets `truncated` if the file was
// larger. Returns false only if the file could not be opened.
bool ReadTextFileCapped(const base::FilePath& path,
                        std::string* content,
                        bool* truncated) {
  std::optional<int64_t> size = base::GetFileSize(path);
  if (!size.has_value()) {
    return false;
  }
  if (base::checked_cast<uint64_t>(*size) <= kMaxReadBytes) {
    *truncated = false;
    return base::ReadFileToString(path, content);
  }
  *truncated = true;
  base::File file(path, base::File::FLAG_OPEN | base::File::FLAG_READ);
  if (!file.IsValid()) {
    return false;
  }
  content->resize(kMaxReadBytes);
  std::optional<size_t> read = file.ReadAtCurrentPos(
      base::as_writable_byte_span(*content).first(kMaxReadBytes));
  if (!read.has_value()) {
    return false;
  }
  content->resize(*read);
  return true;
}

// A rough binary-file check: NUL byte within the first 8 KiB.
bool LooksBinary(std::string_view content) {
  return content.substr(0, 8192).find('\0') != std::string_view::npos;
}

// Depth-limited recursive listing helper. `depth` is remaining levels.
void ListDirInto(const base::FilePath& canonical_root,
                 const base::FilePath& dir,
                 int depth,
                 size_t* count,
                 std::string* out) {
  if (depth <= 0 || *count >= kMaxListEntries) {
    return;
  }
  base::FileEnumerator enumerator(
      dir, /*recursive=*/false,
      base::FileEnumerator::FILES | base::FileEnumerator::DIRECTORIES);
  std::vector<base::FilePath> entries;
  for (base::FilePath path = enumerator.Next(); !path.empty();
       path = enumerator.Next()) {
    entries.push_back(path);
  }
  std::sort(entries.begin(), entries.end());
  for (const auto& path : entries) {
    if (*count >= kMaxListEntries) {
      base::StrAppend(out, {"... (listing truncated)\n"});
      return;
    }
    bool is_dir = base::DirectoryExists(path);
    std::string rel = RelativePosixPath(canonical_root, path);
    base::StrAppend(out, {rel, is_dir ? "/\n" : "\n"});
    ++*count;
    if (is_dir) {
      ListDirInto(canonical_root, path, depth - 1, count, out);
    }
  }
}

}  // namespace

std::optional<base::FilePath> ResolveWithinRoot(
    const base::FilePath& canonical_root,
    const std::string& relative,
    bool require_exists) {
  if (relative.empty()) {
    return canonical_root;
  }
  base::FilePath rel = base::FilePath::FromUTF8Unsafe(relative);

  if (rel.IsAbsolute()) {
    // Coding agents commonly pass absolute paths. Accept them only when they
    // point inside the root; reject anything that escapes it.
    base::FilePath resolved = base::MakeAbsoluteFilePath(rel);
    if (!resolved.empty()) {
      if (resolved == canonical_root || canonical_root.IsParent(resolved)) {
        return resolved;
      }
      return std::nullopt;
    }
    // Absolute but non-existent.
    if (require_exists) {
      return std::nullopt;
    }
    if (rel == canonical_root || canonical_root.IsParent(rel)) {
      return rel;
    }
    return std::nullopt;
  }

  if (rel.ReferencesParent()) {
    return std::nullopt;
  }
  base::FilePath candidate = canonical_root.Append(rel);

  base::FilePath resolved = base::MakeAbsoluteFilePath(candidate);
  if (resolved.empty()) {
    // The target does not exist (or is unreadable). For read ops this is a
    // failure; for create/write ops we return the lexical candidate after a
    // containment check (intermediate components were verified above to not
    // include "..", and the parent is validated separately by write ops).
    if (require_exists) {
      return std::nullopt;
    }
    return candidate;
  }
  // Existing path: the canonicalized (symlink-resolved) form must stay within
  // the root. This is what defeats symlink escapes.
  if (resolved == canonical_root || canonical_root.IsParent(resolved)) {
    return resolved;
  }
  return std::nullopt;
}

FileOpResult ListDir(const base::FilePath& canonical_root,
                     const std::string& relative,
                     int depth) {
  std::optional<base::FilePath> dir =
      ResolveWithinRoot(canonical_root, relative, /*require_exists=*/true);
  if (!dir) {
    return base::unexpected(OutsideRootError(canonical_root, relative));
  }
  if (!base::DirectoryExists(*dir)) {
    return base::unexpected("not a directory: " + relative);
  }
  if (depth <= 0) {
    depth = kDefaultListDepth;
  }
  size_t count = 0;
  std::string out;
  ListDirInto(canonical_root, *dir, depth, &count, &out);
  if (out.empty()) {
    return FileOpResult("(empty directory)");
  }
  return FileOpResult(std::move(out));
}

FileOpResult ReadFile(const base::FilePath& canonical_root,
                      const std::string& relative,
                      std::optional<int> start_line,
                      std::optional<int> end_line) {
  std::optional<base::FilePath> path =
      ResolveWithinRoot(canonical_root, relative, /*require_exists=*/true);
  if (!path) {
    return base::unexpected(OutsideRootError(canonical_root, relative));
  }
  if (base::DirectoryExists(*path)) {
    return base::unexpected("is a directory, not a file: " + relative);
  }
  std::string content;
  bool truncated = false;
  if (!ReadTextFileCapped(*path, &content, &truncated)) {
    return base::unexpected("unable to read file: " + relative);
  }

  std::vector<std::string_view> lines = base::SplitStringPiece(
      content, "\n", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
  // A trailing newline produces a spurious empty final element; drop it.
  if (!lines.empty() && lines.back().empty() && content.ends_with("\n")) {
    lines.pop_back();
  }

  int total = base::checked_cast<int>(lines.size());
  int begin = start_line.value_or(1);
  int end = end_line.value_or(-1);
  if (begin < 1) {
    begin = 1;
  }
  if (end < 0 || end > total) {
    end = total;
  }

  std::string out;
  for (int i = begin; i <= end; ++i) {
    base::StrAppend(&out, {base::StringPrintf("%6d\t", i),
                           std::string(lines[i - 1]), "\n"});
  }
  if (truncated) {
    base::StrAppend(&out, {"... (file truncated at ",
                           base::NumberToString(kMaxReadBytes), " bytes)\n"});
  }
  return FileOpResult(std::move(out));
}

FileOpResult Grep(const base::FilePath& canonical_root,
                  const std::string& relative,
                  const std::string& pattern,
                  const std::string& include) {
  std::optional<base::FilePath> dir =
      ResolveWithinRoot(canonical_root, relative, /*require_exists=*/true);
  if (!dir) {
    return base::unexpected(OutsideRootError(canonical_root, relative));
  }
  re2::RE2 re(pattern);
  if (!re.ok()) {
    return base::unexpected("invalid regular expression: " + re.error());
  }

  std::string out;
  size_t matches = 0;
  bool truncated = false;
  base::FileEnumerator enumerator(*dir, /*recursive=*/true,
                                  base::FileEnumerator::FILES);
  for (base::FilePath path = enumerator.Next();
       !path.empty() && !truncated; path = enumerator.Next()) {
    std::string rel = RelativePosixPath(canonical_root, path);
    if (!include.empty() && !base::MatchPattern(rel, include)) {
      continue;
    }
    std::string content;
    bool file_truncated = false;
    if (!ReadTextFileCapped(path, &content, &file_truncated) ||
        LooksBinary(content)) {
      continue;
    }
    std::vector<std::string_view> lines = base::SplitStringPiece(
        content, "\n", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
    for (size_t i = 0; i < lines.size(); ++i) {
      if (!re2::RE2::PartialMatch(lines[i], re)) {
        continue;
      }
      if (matches >= kMaxGrepMatches) {
        truncated = true;
        break;
      }
      base::StrAppend(&out, {rel, ":", base::NumberToString(i + 1), ": ",
                             std::string(lines[i]), "\n"});
      ++matches;
    }
  }
  if (matches == 0) {
    return FileOpResult("(no matches)");
  }
  if (truncated) {
    base::StrAppend(&out, {"... (results truncated at ",
                           base::NumberToString(kMaxGrepMatches),
                           " matches)\n"});
  }
  return FileOpResult(std::move(out));
}

WriteOutcome::WriteOutcome() = default;
WriteOutcome::WriteOutcome(WriteOutcome&&) = default;
WriteOutcome& WriteOutcome::operator=(WriteOutcome&&) = default;
WriteOutcome::~WriteOutcome() = default;

WriteResult CreateFile(const base::FilePath& canonical_root,
                       const std::string& relative,
                       const std::string& content) {
  if (content.size() > kMaxWriteBytes) {
    return base::unexpected("content exceeds the maximum write size");
  }
  std::optional<base::FilePath> path =
      ResolveWithinRoot(canonical_root, relative, /*require_exists=*/false);
  if (!path) {
    return base::unexpected(OutsideRootError(canonical_root, relative));
  }
  if (base::PathExists(*path)) {
    return base::unexpected("file already exists: " + relative);
  }
  // The parent directory must exist and canonicalize within the root, so a
  // symlinked intermediate directory cannot redirect the new file outside.
  base::FilePath parent = base::MakeAbsoluteFilePath(path->DirName());
  if (parent.empty() ||
      !(parent == canonical_root || canonical_root.IsParent(parent))) {
    return base::unexpected(
        "parent directory does not exist within the workspace");
  }
  if (!base::WriteFile(*path, content)) {
    return base::unexpected("failed to write file: " + relative);
  }
  WriteOutcome outcome;
  outcome.message = "Created " + relative;
  outcome.previous_content = std::nullopt;
  outcome.resolved_path = *path;
  return outcome;
}

WriteResult StrReplace(const base::FilePath& canonical_root,
                       const std::string& relative,
                       const std::string& old_str,
                       const std::string& new_str) {
  if (old_str.empty()) {
    return base::unexpected("old_str must not be empty");
  }
  std::optional<base::FilePath> path =
      ResolveWithinRoot(canonical_root, relative, /*require_exists=*/true);
  if (!path || base::DirectoryExists(*path)) {
    return base::unexpected(OutsideRootError(canonical_root, relative));
  }
  std::string content;
  if (!base::ReadFileToString(*path, &content)) {
    return base::unexpected("unable to read file: " + relative);
  }
  size_t first = content.find(old_str);
  if (first == std::string::npos) {
    return base::unexpected("old_str was not found in " + relative);
  }
  if (content.find(old_str, first + old_str.size()) != std::string::npos) {
    return base::unexpected("old_str is not unique in " + relative +
                            "; include more surrounding context");
  }
  std::string updated = content;
  updated.replace(first, old_str.size(), new_str);
  if (updated.size() > kMaxWriteBytes) {
    return base::unexpected("result exceeds the maximum write size");
  }
  if (!base::WriteFile(*path, updated)) {
    return base::unexpected("failed to write file: " + relative);
  }
  WriteOutcome outcome;
  outcome.message = "Edited " + relative;
  outcome.previous_content = std::move(content);
  outcome.resolved_path = *path;
  return outcome;
}

WriteResult Insert(const base::FilePath& canonical_root,
                   const std::string& relative,
                   int insert_line,
                   const std::string& text) {
  std::optional<base::FilePath> path =
      ResolveWithinRoot(canonical_root, relative, /*require_exists=*/true);
  if (!path || base::DirectoryExists(*path)) {
    return base::unexpected(OutsideRootError(canonical_root, relative));
  }
  std::string content;
  if (!base::ReadFileToString(*path, &content)) {
    return base::unexpected("unable to read file: " + relative);
  }
  const bool trailing_newline = content.ends_with("\n");
  std::vector<std::string> lines = base::SplitString(
      content, "\n", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
  if (trailing_newline && !lines.empty() && lines.back().empty()) {
    lines.pop_back();
  }
  if (insert_line < 0 ||
      insert_line > base::checked_cast<int>(lines.size())) {
    return base::unexpected("insert_line is out of range");
  }
  std::vector<std::string> new_lines = base::SplitString(
      text, "\n", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
  if (text.ends_with("\n") && !new_lines.empty() && new_lines.back().empty()) {
    new_lines.pop_back();
  }
  lines.insert(lines.begin() + insert_line, new_lines.begin(),
               new_lines.end());
  std::string updated = base::JoinString(lines, "\n");
  if (trailing_newline) {
    updated += "\n";
  }
  if (updated.size() > kMaxWriteBytes) {
    return base::unexpected("result exceeds the maximum write size");
  }
  if (!base::WriteFile(*path, updated)) {
    return base::unexpected("failed to write file: " + relative);
  }
  WriteOutcome outcome;
  outcome.message = "Inserted into " + relative;
  outcome.previous_content = std::move(content);
  outcome.resolved_path = *path;
  return outcome;
}

WriteResult AppendFile(const base::FilePath& canonical_root,
                       const std::string& relative,
                       const std::string& content) {
  if (content.size() > kMaxWriteBytes) {
    return base::unexpected("content exceeds the maximum write size");
  }
  std::optional<base::FilePath> path =
      ResolveWithinRoot(canonical_root, relative, /*require_exists=*/false);
  if (!path) {
    return base::unexpected(OutsideRootError(canonical_root, relative));
  }
  const bool existed = base::PathExists(*path);
  std::string previous;
  if (existed) {
    if (base::DirectoryExists(*path)) {
      return base::unexpected("is a directory, not a file: " + relative);
    }
    if (!base::ReadFileToString(*path, &previous)) {
      return base::unexpected("unable to read file: " + relative);
    }
  } else {
    // Creating: the parent must exist and canonicalize within the root, so a
    // symlinked intermediate directory cannot redirect the file outside.
    base::FilePath parent = base::MakeAbsoluteFilePath(path->DirName());
    if (parent.empty() ||
        !(parent == canonical_root || canonical_root.IsParent(parent))) {
      return base::unexpected(
          "parent directory does not exist within the workspace");
    }
  }
  std::string updated = previous + content;
  if (updated.size() > kMaxWriteBytes) {
    return base::unexpected("result exceeds the maximum write size");
  }
  if (!base::WriteFile(*path, updated)) {
    return base::unexpected("failed to write file: " + relative);
  }
  WriteOutcome outcome;
  outcome.message = existed ? ("Appended to " + relative)
                            : ("Created " + relative);
  outcome.previous_content =
      existed ? std::optional<std::string>(std::move(previous)) : std::nullopt;
  outcome.resolved_path = *path;
  return outcome;
}

FileOpResult RestoreFile(const base::FilePath& resolved_path,
                         const std::optional<std::string>& content) {
  if (!content) {
    if (base::PathExists(resolved_path) && !base::DeleteFile(resolved_path)) {
      return base::unexpected("failed to remove file during undo");
    }
    return FileOpResult("Undone (file removed)");
  }
  if (!base::WriteFile(resolved_path, *content)) {
    return base::unexpected("failed to restore file during undo");
  }
  return FileOpResult("Undone");
}

FileOpResult Glob(const base::FilePath& canonical_root,
                  const std::string& relative,
                  const std::string& pattern) {
  std::optional<base::FilePath> dir =
      ResolveWithinRoot(canonical_root, relative, /*require_exists=*/true);
  if (!dir) {
    return base::unexpected(OutsideRootError(canonical_root, relative));
  }
  std::vector<std::string> results;
  base::FileEnumerator enumerator(*dir, /*recursive=*/true,
                                  base::FileEnumerator::FILES);
  for (base::FilePath path = enumerator.Next(); !path.empty();
       path = enumerator.Next()) {
    std::string rel = RelativePosixPath(canonical_root, path);
    // Match either the full relative path or just the file name, so a bare
    // name or a simple "*.ext" from the model finds files at any depth (a
    // leading "**/" is also tolerated this way).
    std::string name = path.BaseName().AsUTF8Unsafe();
    if (base::MatchPattern(rel, pattern) ||
        base::MatchPattern(name, pattern)) {
      results.push_back(rel);
      if (results.size() >= kMaxGlobResults) {
        break;
      }
    }
  }
  if (results.empty()) {
    return FileOpResult("(no matching files)");
  }
  std::sort(results.begin(), results.end());
  return FileOpResult(base::JoinString(results, "\n") + "\n");
}

FileOpResult RepoMap(const base::FilePath& canonical_root,
                     const std::string& relative) {
  std::optional<base::FilePath> dir =
      ResolveWithinRoot(canonical_root, relative, /*require_exists=*/true);
  if (!dir) {
    return base::unexpected(OutsideRootError(canonical_root, relative));
  }

  static constexpr auto kSourceExtensions = std::to_array<std::string_view>(
      {".ts", ".tsx", ".js", ".jsx", ".mjs", ".cc", ".cpp", ".cxx", ".c", ".h",
       ".hpp", ".py", ".rs", ".go", ".java", ".kt", ".swift", ".rb",
       ".mojom"});
  static constexpr size_t kMaxSignaturesPerFile = 40;

  const re2::RE2 def_re(
      R"(^\s*(export\s+)?(default\s+)?((public|private|protected|abstract|static|async|final|pub)\s+)*(class|interface|struct|enum|trait|impl|namespace|module|def|function|func|fn|type)\s)");
  const re2::RE2 export_const_re(
      R"(^\s*export\s+(const|let|var)\s+[A-Za-z_$][\w$]*\s*=)");

  std::string out;
  bool truncated = false;
  base::FileEnumerator enumerator(*dir, /*recursive=*/true,
                                  base::FileEnumerator::FILES);
  for (base::FilePath path = enumerator.Next();
       !path.empty() && !truncated; path = enumerator.Next()) {
    std::string ext = base::ToLowerASCII(path.Extension());
    if (std::ranges::find(kSourceExtensions, ext) == kSourceExtensions.end()) {
      continue;
    }
    std::string content;
    bool file_truncated = false;
    if (!ReadTextFileCapped(path, &content, &file_truncated) ||
        LooksBinary(content)) {
      continue;
    }
    std::vector<std::string_view> lines = base::SplitStringPiece(
        content, "\n", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
    std::string signatures;
    size_t count = 0;
    for (std::string_view line : lines) {
      if (count >= kMaxSignaturesPerFile) {
        break;
      }
      if (re2::RE2::PartialMatch(line, def_re) ||
          re2::RE2::PartialMatch(line, export_const_re)) {
        base::StrAppend(
            &signatures,
            {"  ", base::TrimWhitespaceASCII(line, base::TRIM_ALL), "\n"});
        ++count;
      }
    }
    if (signatures.empty()) {
      continue;
    }
    base::StrAppend(&out, {RelativePosixPath(canonical_root, path), ":\n",
                           signatures});
    if (out.size() >= kMaxReadBytes) {
      base::StrAppend(&out, {"... (repo map truncated)\n"});
      truncated = true;
    }
  }
  if (out.empty()) {
    return FileOpResult("(no source definitions found)");
  }
  return FileOpResult(std::move(out));
}

}  // namespace ai_chat::workspace
