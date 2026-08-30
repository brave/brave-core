// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/common/ai_chat_urls.h"

#include "base/files/file_path.h"
#include "brave/components/ai_chat/core/common/constants.h"
#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ai_chat {

namespace {

base::FilePath MakePath(const char* posix_path) {
#if BUILDFLAG(IS_WIN)
  return base::FilePath(FILE_PATH_LITERAL("C:")).AppendASCII(posix_path + 1);
#else
  return base::FilePath(posix_path);
#endif
}

}  // namespace

TEST(AIChatWorkspaceURLTest, RoundTripsTheFolder) {
  const base::FilePath folder = MakePath("/home/user/projects/leo");
  const GURL url = LeoWorkspaceContentURL(folder);

  EXPECT_EQ(kAIChatLeoWorkspaceUIHost, url.host());
  EXPECT_EQ(folder, LeoWorkspaceFolderFromURL(url));

  // The content URL identifies only the folder; the workspace's own identity
  // is its associated content uuid.
  EXPECT_EQ("/", url.path());
}

TEST(AIChatWorkspaceURLTest, RoundTripsFoldersNeedingEscaping) {
  for (const char* path : {"/home/user/my projects/a&b", "/home/user/100%",
                           "/home/user/a?b#c", "/home/user/ünïcødé"}) {
    const base::FilePath folder = MakePath(path);
    EXPECT_EQ(folder, LeoWorkspaceFolderFromURL(LeoWorkspaceContentURL(folder)))
        << "failed to round trip " << path;
  }
}

TEST(AIChatWorkspaceURLTest, RejectsNonWorkspaceURLs) {
  EXPECT_FALSE(LeoWorkspaceFolderFromURL(GURL()));
  EXPECT_FALSE(
      LeoWorkspaceFolderFromURL(GURL("https://example.com/?folder=/")));
  EXPECT_FALSE(LeoWorkspaceFolderFromURL(
      GURL("chrome-untrusted://aichat-code-sandbox/x?folder=/home/user")));
}

TEST(AIChatWorkspaceURLTest, RejectsWorkspaceURLsWithoutAUsableFolder) {
  // URLs predating the folder being recorded, or cleared by a "clear browsing
  // data" pass, have nothing to reattach to.
  EXPECT_FALSE(
      LeoWorkspaceFolderFromURL(GURL("chrome-untrusted://leo-workspace/")));
  EXPECT_FALSE(LeoWorkspaceFolderFromURL(
      GURL("chrome-untrusted://leo-workspace/some-uuid")));
  EXPECT_FALSE(
      LeoWorkspaceFolderFromURL(GURL("chrome-untrusted://leo-workspace/"
                                     "?folder=")));

  // This has been through a URL and the database, so its shape isn't trusted.
  EXPECT_FALSE(LeoWorkspaceFolderFromURL(
      GURL("chrome-untrusted://leo-workspace/?folder=relative/path")));
  EXPECT_FALSE(LeoWorkspaceFolderFromURL(
      GURL("chrome-untrusted://leo-workspace/?folder=%2Fhome%2F..%2Fetc")));
}

}  // namespace ai_chat
