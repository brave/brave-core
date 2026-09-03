// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/content/browser/workspace_content_source.h"

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/containers/span.h"
#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/memory/ref_counted_memory.h"
#include "base/memory/scoped_refptr.h"
#include "base/strings/escape.h"
#include "base/strings/string_split.h"
#include "base/task/thread_pool.h"
#include "base/uuid.h"
#include "brave/components/ai_chat/content/browser/workspace_content_registry.h"
#include "brave/components/ai_chat/core/common/constants.h"
#include "build/build_config.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"

#if BUILDFLAG(IS_POSIX)
#include <sys/stat.h>
#include <sys/types.h>
#endif

namespace ai_chat {

namespace {

constexpr char kIndexFile[] = "index.html";

std::string_view StripQueryAndRef(std::string_view request_path) {
  return request_path.substr(0, request_path.find_first_of("?#"));
}

std::optional<std::string> DecodePathSegment(std::string_view escaped) {
  // Rejects encoded separators and control bytes rather than decoding them, so
  // "%2f" cannot become a separator after the path has been split.
  std::string unescaped;
  if (!base::UnescapeBinaryURLComponentSafe(
          escaped, /*fail_on_path_separators=*/true, &unescaped)) {
    return std::nullopt;
  }
  // Defence in depth: neither a literal nor an encoded backslash should
  // reach here.
  if (unescaped.find('\\') != std::string::npos) {
    return std::nullopt;
  }
#if BUILDFLAG(IS_WIN)
  // Drive letters ("C:") and NTFS alternate data streams ("notes.txt:ads").
  if (unescaped.find(':') != std::string::npos) {
    return std::nullopt;
  }
#endif  // BUILDFLAG(IS_WIN)
  return unescaped;
}

// base::File::Info only distinguishes directories, so rejecting fifos, sockets
// and device nodes needs a real stat. Not reachable by relative path on
// Windows.
bool IsRegularFile(const base::FilePath& path) {
#if BUILDFLAG(IS_POSIX)
  base::stat_wrapper_t file_info;
  return base::File::Stat(path, &file_info) == 0 && S_ISREG(file_info.st_mode);
#else
  base::File::Info info;
  return base::GetFileInfo(path, &info) && !info.is_directory;
#endif
}

scoped_refptr<base::RefCountedMemory> ReadWorkspaceFileBlocking(
    base::FilePath folder,
    base::FilePath relative_path) {
  std::optional<base::FilePath> resolved =
      ResolveWorkspaceFileBlocking(folder, relative_path);
  if (!resolved) {
    return nullptr;
  }

  // Before reading, so an oversized file costs a stat rather than its size.
  std::optional<int64_t> size = base::GetFileSize(*resolved);
  if (!size || *size > kMaxWorkspaceFileSize) {
    return nullptr;
  }

  std::optional<std::vector<uint8_t>> bytes = base::ReadFileToBytes(*resolved);
  if (!bytes) {
    return nullptr;
  }
  return base::MakeRefCounted<base::RefCountedBytes>(std::move(*bytes));
}

}  // namespace

bool ShouldHandleWorkspaceFileRequest(const std::string& request_path) {
  // Structural only; malformed requests have to be claimed too, so they fail
  // in HandleWorkspaceFileRequest() rather than at the default resource.
  std::vector<std::string_view> segments =
      base::SplitStringPiece(StripQueryAndRef(request_path), "/",
                             base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  return segments.size() >= 2 && segments[1] == kAIChatLeoWorkspaceFilesSegment;
}

void HandleWorkspaceFileRequest(
    base::WeakPtr<content::BrowserContext> browser_context,
    const std::string& request_path,
    content::URLDataSource::GotDataCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!browser_context) {
    std::move(callback).Run(nullptr);
    return;
  }

  std::optional<WorkspaceFileRequest> parsed =
      ParseWorkspaceFileRequestPath(request_path);
  if (!parsed) {
    std::move(callback).Run(nullptr);
    return;
  }

  // Unknown uuids fail closed, so an iframe left over from a closed
  // conversation resolves to nothing rather than to the current folder.
  WorkspaceContentRegistry* registry =
      WorkspaceContentRegistry::Get(browser_context.get());
  const base::FilePath* folder =
      registry ? registry->GetFolder(parsed->uuid) : nullptr;
  if (!folder) {
    std::move(callback).Run(nullptr);
    return;
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::USER_BLOCKING,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(&ReadWorkspaceFileBlocking, *folder,
                     parsed->relative_path),
      std::move(callback));
}

std::optional<WorkspaceFileRequest> ParseWorkspaceFileRequestPath(
    std::string_view request_path) {
  const std::string_view path = StripQueryAndRef(request_path);

  // SPLIT_WANT_NONEMPTY drops the trailing empty segment, so note it first.
  const bool is_directory = path.ends_with("/");

  std::vector<std::string_view> segments = base::SplitStringPiece(
      path, "/", base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  if (segments.size() < 2 || segments[1] != kAIChatLeoWorkspaceFilesSegment) {
    return std::nullopt;
  }

  // Registered uuids are generated lowercase, so anything else could not match.
  if (!base::Uuid::ParseLowercase(segments[0]).is_valid()) {
    return std::nullopt;
  }

  WorkspaceFileRequest parsed;
  parsed.uuid = std::string(segments[0]);

  for (std::string_view segment : base::span(segments).subspan(2u)) {
    std::optional<std::string> decoded_segment = DecodePathSegment(segment);
    if (!decoded_segment) {
      return std::nullopt;
    }
    // Not AppendASCII: generated files can legitimately have non-ASCII names.
    parsed.relative_path = parsed.relative_path.Append(
        base::FilePath::FromUTF8Unsafe(*decoded_segment));
  }

  // A directory root serves its index file.
  if (parsed.relative_path.empty() || is_directory) {
    parsed.relative_path =
        parsed.relative_path.Append(base::FilePath::FromUTF8Unsafe(kIndexFile));
  }

  return parsed;
}

std::optional<base::FilePath> ResolveWorkspaceFileBlocking(
    const base::FilePath& folder,
    const base::FilePath& relative_path) {
  // Guards a future caller: an absolute or parent-referencing path would make
  // Append() escape the folder (and DCHECK).
  if (folder.empty() || relative_path.empty() || relative_path.IsAbsolute() ||
      relative_path.ReferencesParent()) {
    return std::nullopt;
  }

  // Resolve both sides so a symlink inside the folder cannot point outside it.
  // MakeAbsoluteFilePath also fails for paths that do not exist.
  base::FilePath resolved_folder = base::MakeAbsoluteFilePath(folder);
  base::FilePath resolved_file =
      base::MakeAbsoluteFilePath(folder.Append(relative_path));
  if (resolved_folder.empty() || resolved_file.empty()) {
    return std::nullopt;
  }
  // IsParent() is a string comparison, only sound because both sides have
  // been through MakeAbsoluteFilePath().
  if (!resolved_folder.IsParent(resolved_file)) {
    return std::nullopt;
  }

  // Reading a fifo would block a thread-pool thread until a writer appeared;
  // sockets and device nodes are equally not previewable content.
  if (!IsRegularFile(resolved_file)) {
    return std::nullopt;
  }
  if (!base::PathIsReadable(resolved_file)) {
    return std::nullopt;
  }

  return resolved_file;
}

}  // namespace ai_chat
