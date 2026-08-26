// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/ai_chat/leo_workspace_content_ui.h"

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/memory/ref_counted_memory.h"
#include "base/strings/escape.h"
#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "base/task/thread_pool.h"
#include "brave/common/webui_url_constants.h"
#include "brave/components/ai_chat/content/browser/workspace_content_registry.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/constants.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/constants/webui_url_constants.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "url/gurl.h"

namespace ai_chat {

namespace {

// Served when a folder root is requested, matching the conventional behaviour
// of a static file server.
constexpr char kIndexFile[] = "index.html";

// Decodes one URL path segment and rejects anything that could be used to walk
// out of the workspace folder. Escaped separators and control characters are
// rejected rather than decoded, so "%2e%2e%2f" cannot become "../".
bool DecodePathSegment(std::string_view escaped, std::string* decoded) {
  if (escaped.empty()) {
    return false;
  }
  std::string unescaped;
  if (!base::UnescapeBinaryURLComponentSafe(
          escaped, /*fail_on_path_separators=*/true, &unescaped)) {
    return false;
  }
  if (unescaped.empty() || unescaped == "." || unescaped == "..") {
    return false;
  }
  if (unescaped.find('/') != std::string::npos ||
      unescaped.find('\\') != std::string::npos) {
    return false;
  }
  *decoded = std::move(unescaped);
  return true;
}

void OnFileRead(content::WebUIDataSource::GotDataCallback callback,
                std::string contents) {
  std::move(callback).Run(
      base::MakeRefCounted<base::RefCountedString>(std::move(contents)));
}

// Resolves |path| against the folder registered for its uuid, reading the file
// on the thread pool. Unknown uuids and unreadable files both yield empty
// contents, so a preview iframe left over from a closed conversation fails
// closed rather than falling back to some other workspace's folder.
void HandleRequest(content::BrowserContext* browser_context,
                   const std::string& path,
                   content::WebUIDataSource::GotDataCallback callback) {
  std::string uuid;
  base::FilePath relative_path;
  if (!ParseWorkspaceContentPath(path, &uuid, &relative_path)) {
    std::move(callback).Run(base::MakeRefCounted<base::RefCountedString>());
    return;
  }

  base::FilePath folder =
      WorkspaceContentRegistry::GetOrCreate(browser_context)->GetFolder(uuid);
  if (folder.empty()) {
    std::move(callback).Run(base::MakeRefCounted<base::RefCountedString>());
    return;
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(&ReadWorkspaceFileBlocking, folder, relative_path),
      base::BindOnce(&OnFileRead, std::move(callback)));
}

}  // namespace

bool ParseWorkspaceContentPath(const std::string& request_path,
                               std::string* uuid,
                               base::FilePath* relative_path) {
  CHECK(uuid);
  CHECK(relative_path);

  // |request_path| still carries any query and fragment, so round-trip it
  // through GURL to get just the path. This also collapses dot segments, but
  // the per-segment checks below are what actually enforce confinement.
  GURL url(base::StrCat({kAIChatLeoWorkspaceContentUIURL, request_path}));
  if (!url.is_valid()) {
    return false;
  }

  std::vector<std::string_view> segments = base::SplitStringPiece(
      url.path(), "/", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  if (segments.empty()) {
    return false;
  }

  std::string decoded_uuid;
  if (!DecodePathSegment(segments.front(), &decoded_uuid)) {
    return false;
  }

  base::FilePath decoded_path;
  for (size_t i = 1; i < segments.size(); ++i) {
    std::string decoded_segment;
    if (!DecodePathSegment(segments[i], &decoded_segment)) {
      return false;
    }
    // Not AppendASCII: generated files can legitimately have non-ASCII names.
    decoded_path =
        decoded_path.Append(base::FilePath::FromUTF8Unsafe(decoded_segment));
  }

  // A request for the workspace root, or for a path ending in a separator,
  // serves that directory's index file.
  if (decoded_path.empty() || url.path().ends_with("/")) {
    decoded_path =
        decoded_path.Append(base::FilePath::FromUTF8Unsafe(kIndexFile));
  }

  *uuid = std::move(decoded_uuid);
  *relative_path = std::move(decoded_path);
  return true;
}

std::string ReadWorkspaceFileBlocking(const base::FilePath& folder,
                                      const base::FilePath& relative_path) {
  if (folder.empty() || relative_path.empty() || relative_path.IsAbsolute() ||
      relative_path.ReferencesParent()) {
    return std::string();
  }

  // Resolve both sides so that a symlink inside the workspace folder cannot
  // point at a file outside it. MakeAbsoluteFilePath fails for paths that do
  // not exist, which also gives us the "not found" case for free.
  base::FilePath resolved_folder = base::MakeAbsoluteFilePath(folder);
  base::FilePath resolved_file =
      base::MakeAbsoluteFilePath(folder.Append(relative_path));
  if (resolved_folder.empty() || resolved_file.empty()) {
    return std::string();
  }
  if (!resolved_folder.IsParent(resolved_file)) {
    return std::string();
  }

  std::string contents;
  if (!base::ReadFileToString(resolved_file, &contents)) {
    return std::string();
  }
  return contents;
}

bool LeoWorkspaceContentUIConfig::IsWebUIEnabled(
    content::BrowserContext* browser_context) {
  return IsAIChatEnabled(user_prefs::UserPrefs::Get(browser_context)) &&
         base::FeatureList::IsEnabled(features::kAIChatWorkspaceTools);
}

std::unique_ptr<content::WebUIController>
LeoWorkspaceContentUIConfig::CreateWebUIController(content::WebUI* web_ui,
                                                   const GURL& url) {
  return std::make_unique<LeoWorkspaceContentUI>(web_ui);
}

LeoWorkspaceContentUIConfig::LeoWorkspaceContentUIConfig()
    : WebUIConfig(content::kChromeUIUntrustedScheme,
                  kAIChatLeoWorkspaceContentUIHost) {}

LeoWorkspaceContentUIConfig::~LeoWorkspaceContentUIConfig() = default;

LeoWorkspaceContentUI::LeoWorkspaceContentUI(content::WebUI* web_ui)
    : ui::UntrustedWebUIController(web_ui) {
  auto* browser_context = web_ui->GetWebContents()->GetBrowserContext();
  auto* source = content::WebUIDataSource::CreateAndAdd(
      browser_context, kAIChatLeoWorkspaceContentUIURL);

  // Every path is a workspace file; there are no bundled resources here.
  source->SetRequestFilter(
      base::BindRepeating([](const std::string& path) { return true; }),
      base::BindRepeating(&HandleRequest, browser_context));

  // The content served here is model-authored, so it is treated as hostile.
  // 'unsafe-inline' for script and style is deliberate - generated pages
  // routinely use both, and without it a preview is not representative. What
  // keeps that acceptable is that this origin has no bindings, no File System
  // Access grant, and no way to reach the network: default-src/connect-src
  // confine every load to the workspace folder itself, so scripts here have
  // nothing to exfiltrate to.
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::DefaultSrc, "default-src 'self';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      "script-src 'self' 'unsafe-inline' 'unsafe-eval';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::StyleSrc,
      "style-src 'self' 'unsafe-inline';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ImgSrc, "img-src 'self' data: blob:;");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FontSrc, "font-src 'self' data:;");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::MediaSrc,
      "media-src 'self' data: blob:;");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ConnectSrc, "connect-src 'none';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ObjectSrc, "object-src 'none';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameSrc, "frame-src 'self';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::WorkerSrc, "worker-src 'none';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FormAction, "form-action 'none';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::BaseURI, "base-uri 'none';");
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameAncestors,
      absl::StrFormat("frame-ancestors %s %s;", kAIChatUIURL,
                      kAIChatUntrustedConversationUIURL));

  // Generated pages are ordinary HTML and will assign innerHTML, set script
  // src, and so on. The WebUI default of require-trusted-types-for 'script'
  // would break all of that, and buys nothing for content we already treat as
  // untrusted.
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::RequireTrustedTypesFor, std::string());
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::TrustedTypes, std::string());
}

LeoWorkspaceContentUI::~LeoWorkspaceContentUI() = default;

}  // namespace ai_chat
