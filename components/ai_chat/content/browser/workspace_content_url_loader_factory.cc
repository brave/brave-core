// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/content/browser/workspace_content_url_loader_factory.h"

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/self_deleting.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/escape.h"
#include "base/strings/string_split.h"
#include "base/task/thread_pool.h"
#include "brave/components/ai_chat/content/browser/workspace_content_registry.h"
#include "brave/components/ai_chat/core/common/constants.h"
#include "brave/components/constants/webui_url_constants.h"
#include "build/build_config.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/file_url_loader.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "net/base/filename_util.h"
#include "net/base/mime_util.h"
#include "net/base/net_errors.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/self_deleting_url_loader_factory.h"
#include "services/network/public/cpp/url_loader_completion_status.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "url/gurl.h"
#include "url/origin.h"

#if BUILDFLAG(IS_POSIX)
#include <sys/stat.h>
#include <sys/types.h>
#endif

namespace ai_chat {

namespace {

constexpr char kIndexFile[] = "index.html";

void CompleteWithError(
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    int net_error) {
  mojo::Remote<network::mojom::URLLoaderClient>(std::move(client))
      ->OnComplete(network::URLLoaderCompletionStatus(net_error));
}

std::optional<std::string> DecodePathSegment(std::string_view escaped) {
  // Rejects encoded separators and control bytes rather than decoding them, so
  // "%2f" cannot become a separator after the path has been split.
  std::string unescaped;
  if (!base::UnescapeBinaryURLComponentSafe(
          escaped, /*fail_on_path_separators=*/true, &unescaped)) {
    return std::nullopt;
  }
  // Defence in depth: as a registered standard scheme GURL has already turned
  // any literal backslash into a separator, and UnescapeBinaryURLComponentSafe
  // rejects the encoded form. Neither should reach here.
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
// and device nodes needs a real stat. They are not reachable by a relative path
// on Windows, where the directory check is enough.
bool IsRegularFile(const base::FilePath& path) {
#if BUILDFLAG(IS_POSIX)
  base::stat_wrapper_t file_info;
  return base::File::Stat(path, &file_info) == 0 && S_ISREG(file_info.st_mode);
#else
  base::File::Info info;
  return base::GetFileInfo(path, &info) && !info.is_directory;
#endif
}

// Serving the policy on the response, rather than relying on how the frame
// happens to be embedded, means it holds however the URL is reached.
scoped_refptr<net::HttpResponseHeaders> BuildResponseHeaders(
    const base::FilePath& file_path) {
  auto headers =
      base::MakeRefCounted<net::HttpResponseHeaders>("HTTP/1.1 200 OK");

  // GetWellKnownMimeType, not GetMimeType: the latter consults the platform
  // registry, letting whatever the user has installed decide how a file is
  // interpreted.
  std::string mime_type;
  if (!net::GetWellKnownMimeTypeFromFile(file_path, &mime_type)) {
    mime_type = "application/octet-stream";
  }
  headers->SetHeader(net::HttpRequestHeaders::kContentType, mime_type);
  headers->SetHeader("X-Content-Type-Options", "nosniff");

  // The model rewrites these files as the conversation goes.
  headers->SetHeader("Cache-Control", "no-store");

  // 'unsafe-inline'/'unsafe-eval' are deliberate: generated pages routinely use
  // both, and this origin holds nothing worth stealing - one workspace's
  // folder, no bindings, no File System Access grant.
  //
  // Deliberately no sandbox directive: it would force an opaque origin and undo
  // the per-workspace separation this scheme exists for. Containing top-level
  // navigation and popups is the embedding iframe's job.
  //
  // frame-ancestors matches on origin; serialising the constants drops their
  // trailing slash, which Blink would otherwise report as an ignored path.
  headers->SetHeader(
      "Content-Security-Policy",
      absl::StrFormat(
          "default-src 'self';"
          "script-src 'self' 'unsafe-inline' 'unsafe-eval';"
          "style-src 'self' 'unsafe-inline';"
          "img-src 'self' data: blob:;"
          "font-src 'self' data:;"
          "media-src 'self' data: blob:;"
          "connect-src 'none';"
          "object-src 'none';"
          "frame-src 'self';"
          "worker-src 'none';"
          "form-action 'none';"
          "base-uri 'none';"
          "frame-ancestors %s %s;",
          url::Origin::Create(GURL(kAIChatUIURL)).Serialize(),
          url::Origin::Create(GURL(kAIChatUntrustedConversationUIURL))
              .Serialize()));

  return headers;
}

void OnFileResolved(network::ResourceRequest request,
                    mojo::PendingReceiver<network::mojom::URLLoader> loader,
                    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
                    std::optional<base::FilePath> resolved_path) {
  if (!resolved_path) {
    CompleteWithError(std::move(client), net::ERR_FILE_NOT_FOUND);
    return;
  }

  auto headers = BuildResponseHeaders(*resolved_path);

  // Safe because the navigation commits the original brave-leo-workspace://
  // URL, which the browser tracks separately; the response head carries no URL
  // of its own.
  request.url = net::FilePathToFileURL(*resolved_path);

  content::CreateFileURLLoaderBypassingSecurityChecks(
      request, std::move(loader), std::move(client), /*observer=*/nullptr,
      /*allow_directory_listing=*/false, std::move(headers));
}

class WorkspaceContentURLLoaderFactory
    : public network::SelfDeletingURLLoaderFactory {
 public:
  WorkspaceContentURLLoaderFactory(const WorkspaceContentURLLoaderFactory&) =
      delete;
  WorkspaceContentURLLoaderFactory& operator=(
      const WorkspaceContentURLLoaderFactory&) = delete;

  static mojo::PendingRemote<network::mojom::URLLoaderFactory> Create(
      content::BrowserContext* browser_context,
      std::optional<std::string> restrict_to_uuid) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    CHECK(browser_context);

    mojo::PendingRemote<network::mojom::URLLoaderFactory> pending_remote;
    if (browser_context->ShutdownStarted()) {
      return pending_remote;
    }

    base::MakeSelfDeleting<WorkspaceContentURLLoaderFactory>(
        browser_context, std::move(restrict_to_uuid),
        pending_remote.InitWithNewPipeAndPassReceiver());
    return pending_remote;
  }

  WorkspaceContentURLLoaderFactory(
      content::BrowserContext* browser_context,
      std::optional<std::string> restrict_to_uuid,
      mojo::PendingReceiver<network::mojom::URLLoaderFactory> factory_receiver,
      base::SelfDeletingPassKey key)
      : network::SelfDeletingURLLoaderFactory(std::move(factory_receiver), key),
        restrict_to_uuid_(std::move(restrict_to_uuid)),
        browser_context_(browser_context->GetWeakPtr()) {}

 private:
  ~WorkspaceContentURLLoaderFactory() override = default;

  // network::mojom::URLLoaderFactory:
  void CreateLoaderAndStart(
      mojo::PendingReceiver<network::mojom::URLLoader> loader,
      int32_t request_id,
      uint32_t options,
      const network::ResourceRequest& request,
      mojo::PendingRemote<network::mojom::URLLoaderClient> client,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation)
      override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

    // The remote is held by a renderer, so it can outlive the profile.
    if (browser_context_.WasInvalidated()) {
      DVLOG(1) << "Context has been destroyed";
      CompleteWithError(std::move(client), net::ERR_FAILED);
      // Destroys `this`.
      DisconnectReceiversAndDestroy();
      return;
    }

    std::optional<WorkspaceContentPath> parsed =
        ParseWorkspaceContentURL(request.url);
    if (!parsed) {
      CompleteWithError(std::move(client), net::ERR_INVALID_URL);
      return;
    }

    // A factory handed to a workspace frame serves only that workspace, so a
    // preview cannot read another's files by naming its uuid.
    if (restrict_to_uuid_ && *restrict_to_uuid_ != parsed->uuid) {
      CompleteWithError(std::move(client), net::ERR_FILE_NOT_FOUND);
      return;
    }

    // Unknown uuids fail closed, so an iframe left over from a closed
    // conversation resolves to nothing rather than to the current folder.
    WorkspaceContentRegistry* registry =
        WorkspaceContentRegistry::Get(browser_context_.get());
    const base::FilePath* folder =
        registry ? registry->GetFolder(parsed->uuid) : nullptr;
    if (!folder) {
      CompleteWithError(std::move(client), net::ERR_FILE_NOT_FOUND);
      return;
    }

    base::ThreadPool::PostTaskAndReplyWithResult(
        FROM_HERE,
        {base::MayBlock(), base::TaskPriority::USER_BLOCKING,
         base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
        base::BindOnce(&ResolveWorkspaceFileBlocking, *folder,
                       parsed->relative_path),
        base::BindOnce(&OnFileResolved, request, std::move(loader),
                       std::move(client)));
  }

  std::optional<std::string> restrict_to_uuid_;
  base::WeakPtr<content::BrowserContext> browser_context_;
};

}  // namespace

mojo::PendingRemote<network::mojom::URLLoaderFactory>
CreateWorkspaceContentURLLoaderFactory(
    content::BrowserContext* browser_context,
    std::optional<std::string> restrict_to_uuid) {
  return WorkspaceContentURLLoaderFactory::Create(browser_context,
                                                  std::move(restrict_to_uuid));
}

std::optional<WorkspaceContentPath> ParseWorkspaceContentURL(const GURL& url) {
  if (!url.is_valid() || !url.SchemeIs(kLeoWorkspaceContentScheme) ||
      url.host().empty()) {
    return std::nullopt;
  }

  // Already canonicalised and lowercased by GURL. Anything that is not a plain
  // uuid will simply not match a registration.
  WorkspaceContentPath parsed;
  parsed.uuid = url.GetHost();

  std::vector<std::string_view> segments = base::SplitStringPiece(
      url.path(), "/", base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  for (std::string_view segment : segments) {
    std::optional<std::string> decoded_segment = DecodePathSegment(segment);
    if (!decoded_segment) {
      return std::nullopt;
    }
    // Not AppendASCII: generated files can legitimately have non-ASCII names.
    parsed.relative_path = parsed.relative_path.Append(
        base::FilePath::FromUTF8Unsafe(*decoded_segment));
  }

  // A directory root serves its index file.
  if (parsed.relative_path.empty() || url.path().ends_with("/")) {
    parsed.relative_path =
        parsed.relative_path.Append(base::FilePath::FromUTF8Unsafe(kIndexFile));
  }

  return parsed;
}

std::optional<base::FilePath> ResolveWorkspaceFileBlocking(
    const base::FilePath& folder,
    const base::FilePath& relative_path) {
  // An absolute or parent-referencing relative_path would make Append() escape
  // the folder (and DCHECK); the caller never produces one, so this is a guard
  // against a future caller rather than against the URL.
  if (folder.empty() || relative_path.empty() || relative_path.IsAbsolute() ||
      relative_path.ReferencesParent()) {
    return std::nullopt;
  }

  // Resolve both sides so a symlink inside the folder cannot point outside it.
  // MakeAbsoluteFilePath also fails for paths that do not exist, which covers
  // "not found".
  base::FilePath resolved_folder = base::MakeAbsoluteFilePath(folder);
  base::FilePath resolved_file =
      base::MakeAbsoluteFilePath(folder.Append(relative_path));
  if (resolved_folder.empty() || resolved_file.empty()) {
    return std::nullopt;
  }
  // IsParent() is a pure string comparison, which is only sound because both
  // sides have been through MakeAbsoluteFilePath().
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
