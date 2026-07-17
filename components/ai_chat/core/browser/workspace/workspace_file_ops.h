// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_WORKSPACE_WORKSPACE_FILE_OPS_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_WORKSPACE_WORKSPACE_FILE_OPS_H_

#include <optional>
#include <string>

#include "base/files/file_path.h"
#include "base/types/expected.h"

// Pure, synchronous, //base-only implementations of the "workspace" local file
// tools. These are the heart of the feature's security model: every operation
// is confined to a user-selected root directory via ResolveWithinRoot(). They
// are deliberately free functions (no //content, no threading) so they can be
// unit-tested directly and posted to a background sequence by WorkspaceService.
//
// The string in a successful result is model-facing text (e.g. a directory
// listing or a file's contents); the error string is a short, model-readable
// explanation (e.g. "path escapes the workspace root").

namespace ai_chat::workspace {

// Output caps. Results feed into the model context, so they are bounded.
inline constexpr size_t kMaxReadBytes = 256 * 1024;
inline constexpr size_t kMaxGrepMatches = 100;
inline constexpr size_t kMaxGlobResults = 200;
inline constexpr size_t kMaxListEntries = 1000;
inline constexpr int kDefaultListDepth = 2;
// Upper bound on the size of a file produced by a write op.
inline constexpr size_t kMaxWriteBytes = 2 * 1024 * 1024;

using FileOpResult = base::expected<std::string, std::string>;

// The result of a mutating op. `previous_content` is what the file held before
// the edit (std::nullopt if the file was newly created), captured so the edit
// can be undone. `resolved_path` is the confined absolute path that was written.
struct WriteOutcome {
  WriteOutcome();
  WriteOutcome(WriteOutcome&&);
  WriteOutcome& operator=(WriteOutcome&&);
  ~WriteOutcome();

  std::string message;
  std::optional<std::string> previous_content;
  base::FilePath resolved_path;
};

using WriteResult = base::expected<WriteOutcome, std::string>;

// Resolves `relative` (a workspace-relative, '/'-separated path from the model)
// against `canonical_root` (an already-canonicalized, symlink-free absolute
// path), guaranteeing the result stays within the root. Returns std::nullopt if
// `relative` is absolute, contains a ".." component, or (when it resolves to an
// existing path) points outside the root via symlinks. When `require_exists` is
// true, a non-existent target yields std::nullopt; when false (for create/write
// paths), the lexical candidate is returned after a containment check.
std::optional<base::FilePath> ResolveWithinRoot(
    const base::FilePath& canonical_root,
    const std::string& relative,
    bool require_exists);

// Lists directory entries under `relative` (empty = root) up to `depth` levels
// deep. Directories are suffixed with '/'.
FileOpResult ListDir(const base::FilePath& canonical_root,
                     const std::string& relative,
                     int depth);

// Reads a text file at `relative`, prefixing 1-indexed line numbers (cat -n
// style). If provided, only lines in [start_line, end_line] are returned
// (end_line < 0 means through end of file).
FileOpResult ReadFile(const base::FilePath& canonical_root,
                      const std::string& relative,
                      std::optional<int> start_line,
                      std::optional<int> end_line);

// Searches file contents under `relative` (empty = root) for the RE2 regex
// `pattern`. `include` (optional) is a glob filtering candidate files by their
// workspace-relative path. Returns "path:line: text" matches, capped.
FileOpResult Grep(const base::FilePath& canonical_root,
                  const std::string& relative,
                  const std::string& pattern,
                  const std::string& include);

// Finds files whose workspace-relative path matches the glob `pattern`
// (base::MatchPattern semantics). Returns matching paths, capped.
FileOpResult Glob(const base::FilePath& canonical_root,
                  const std::string& relative,
                  const std::string& pattern);

// Produces an Aider-style structural outline of the source files under
// `relative` (empty = root): for each recognized source file, its path plus the
// top-level definition lines (classes, functions, etc.) found via lightweight
// pattern matching. Budgeted to a bounded output size.
FileOpResult RepoMap(const base::FilePath& canonical_root,
                     const std::string& relative);

// Creates a new file at `relative` with `content`. Fails if it already exists
// or if its parent directory is missing/outside the root.
WriteResult CreateFile(const base::FilePath& canonical_root,
                       const std::string& relative,
                       const std::string& content);

// Replaces the single, unique occurrence of `old_str` in `relative` with
// `new_str`. Fails if `old_str` is absent or matches more than once.
WriteResult StrReplace(const base::FilePath& canonical_root,
                       const std::string& relative,
                       const std::string& old_str,
                       const std::string& new_str);

// Inserts `text` into `relative` after 1-indexed line `insert_line` (0 inserts
// at the start of the file).
WriteResult Insert(const base::FilePath& canonical_root,
                   const std::string& relative,
                   int insert_line,
                   const std::string& text);

// Appends `content` to the end of `relative`, creating the file if it does not
// exist. Lets a model build up a large file across several smaller calls.
WriteResult AppendFile(const base::FilePath& canonical_root,
                       const std::string& relative,
                       const std::string& content);

// Restores a previously-edited file: writes `content` back to `resolved_path`,
// or deletes the file if `content` is std::nullopt (the edit had created it).
// `resolved_path` must be an absolute path previously returned by a write op.
FileOpResult RestoreFile(const base::FilePath& resolved_path,
                         const std::optional<std::string>& content);

}  // namespace ai_chat::workspace

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_WORKSPACE_WORKSPACE_FILE_OPS_H_
