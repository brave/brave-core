// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/common/ai_chat_urls.h"

#include <optional>
#include <string>
#include <string_view>

#include "base/files/file_path.h"
#include "base/strings/escape.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "brave/components/ai_chat/core/common/constants.h"
#include "brave/components/constants/webui_url_constants.h"
#include "url/gurl.h"
#include "url/third_party/mozilla/url_parse.h"

namespace ai_chat {

namespace {

constexpr std::string_view kLeoWorkspaceFolderQueryKey = "folder";

}  // namespace

GURL TabAssociatedConversationUrl() {
  return GURL(base::StrCat({kAIChatUIURL, "tab"}));
}

GURL ConversationUrl(std::string_view conversation_uuid) {
  return GURL(base::StrCat({kAIChatUIURL, conversation_uuid}));
}

std::string_view ConversationUUIDFromURL(const GURL& url) {
  return base::TrimString(url.path(), "/", base::TrimPositions::TRIM_ALL);
}

GURL LeoWorkspaceContentURL(const base::FilePath& folder) {
  // Note: a path that isn't valid UTF-8 is lossy here, which fails safe: the
  // recovered path won't match an existing directory, so nothing is reattached.
  std::string query =
      base::StrCat({kLeoWorkspaceFolderQueryKey, "=",
                    base::EscapeQueryParamValue(folder.AsUTF8Unsafe(),
                                                /*use_plus=*/false)});
  GURL::Replacements replacements;
  replacements.SetQueryStr(query);
  return GURL(kAIChatLeoWorkspaceUIURL).ReplaceComponents(replacements);
}

std::optional<base::FilePath> LeoWorkspaceFolderFromURL(const GURL& url) {
  if (!base::StartsWith(url.spec(), kAIChatLeoWorkspaceUIURL)) {
    return std::nullopt;
  }

  std::string_view query_piece = url.query();
  url::Component query(0, query_piece.length());
  url::Component key;
  url::Component value;
  while (url::ExtractQueryKeyValue(query_piece, &query, &key, &value)) {
    if (query_piece.substr(key.begin, key.len) != kLeoWorkspaceFolderQueryKey) {
      continue;
    }
    base::FilePath folder =
        base::FilePath::FromUTF8Unsafe(base::UnescapeBinaryURLComponent(
            query_piece.substr(value.begin, value.len)));
    // This has been through a URL and the database, so don't trust its shape.
    if (folder.empty() || !folder.IsAbsolute() || folder.ReferencesParent()) {
      return std::nullopt;
    }
    return folder;
  }
  return std::nullopt;
}

}  // namespace ai_chat
