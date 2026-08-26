// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/ai_chat/leo_workspace_content_ui.h"

#include <string>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

namespace {

constexpr char kUuid[] = "6b1b3f1e-0b7a-4f27-9a2f-2f2b9d4e5a10";

base::FilePath ParsedPath(const std::string& request_path) {
  std::string uuid;
  base::FilePath relative_path;
  EXPECT_TRUE(ParseWorkspaceContentPath(request_path, &uuid, &relative_path))
      << request_path;
  EXPECT_EQ(kUuid, uuid);
  return relative_path;
}

void ExpectRejected(const std::string& request_path) {
  std::string uuid;
  base::FilePath relative_path;
  EXPECT_FALSE(ParseWorkspaceContentPath(request_path, &uuid, &relative_path))
      << request_path;
}

}  // namespace

TEST(LeoWorkspaceContentUITest, ParsesUuidAndRelativePath) {
  EXPECT_EQ(base::FilePath::FromASCII("index.html"),
            ParsedPath(std::string(kUuid) + "/index.html"));
  EXPECT_EQ(base::FilePath::FromASCII("assets").AppendASCII("app.js"),
            ParsedPath(std::string(kUuid) + "/assets/app.js"));
}

TEST(LeoWorkspaceContentUITest, DirectoryRootServesIndex) {
  // Bare uuid, trailing slash, and a subdirectory all fall back to index.html.
  EXPECT_EQ(base::FilePath::FromASCII("index.html"), ParsedPath(kUuid));
  EXPECT_EQ(base::FilePath::FromASCII("index.html"),
            ParsedPath(std::string(kUuid) + "/"));
  EXPECT_EQ(base::FilePath::FromASCII("docs").AppendASCII("index.html"),
            ParsedPath(std::string(kUuid) + "/docs/"));
}

TEST(LeoWorkspaceContentUITest, StripsQueryAndFragment) {
  // URLToRequestPath hands us the query and fragment as part of the path.
  EXPECT_EQ(base::FilePath::FromASCII("app.js"),
            ParsedPath(std::string(kUuid) + "/app.js?v=2"));
  EXPECT_EQ(base::FilePath::FromASCII("index.html"),
            ParsedPath(std::string(kUuid) + "/index.html#section"));
}

TEST(LeoWorkspaceContentUITest, DecodesPercentEscapes) {
  EXPECT_EQ(base::FilePath::FromASCII("my file.txt"),
            ParsedPath(std::string(kUuid) + "/my%20file.txt"));
}

TEST(LeoWorkspaceContentUITest, RejectsEscapedSeparators) {
  // Escaped separators would smuggle extra path components past URL
  // canonicalisation, so they have to be rejected rather than decoded.
  ExpectRejected(std::string(kUuid) + "/%2E%2E%2Fsecret");
  ExpectRejected(std::string(kUuid) + "/sub%2f..%2fsecret");
  ExpectRejected(std::string(kUuid) + "/sub%5c..%5csecret");
}

TEST(LeoWorkspaceContentUITest, TraversalCannotEscapeWorkspace) {
  // These parse successfully rather than being rejected: URL canonicalisation
  // has already resolved the dot segments, which consumes the uuid segment. The
  // property that matters is that whatever comes out is confined - and that the
  // uuid no longer identifies the workspace that was being targeted.
  for (const char* input :
       {"/../secret", "/sub/../../secret", "/%2e%2e/secret", "/./../secret"}) {
    const std::string request_path = std::string(kUuid) + input;
    std::string uuid;
    base::FilePath relative_path;
    if (!ParseWorkspaceContentPath(request_path, &uuid, &relative_path)) {
      continue;
    }
    EXPECT_NE(kUuid, uuid) << request_path;
    EXPECT_FALSE(relative_path.IsAbsolute()) << request_path;
    EXPECT_FALSE(relative_path.ReferencesParent()) << request_path;
  }
}

TEST(LeoWorkspaceContentUITest, InteriorDotSegmentsStayInWorkspace) {
  // A dot segment that does not escape the workspace keeps the uuid intact and
  // simply normalises the path.
  EXPECT_EQ(base::FilePath::FromASCII("b"),
            ParsedPath(std::string(kUuid) + "/a/../b"));
}

TEST(LeoWorkspaceContentUITest, RejectsEmptyPath) {
  ExpectRejected("");
  ExpectRejected("/");
}

TEST(LeoWorkspaceContentUITest, ReadsFileWithinFolder) {
  base::ScopedTempDir dir;
  ASSERT_TRUE(dir.CreateUniqueTempDir());
  ASSERT_TRUE(
      base::WriteFile(dir.GetPath().AppendASCII("index.html"), "<h1>hi</h1>"));

  EXPECT_EQ("<h1>hi</h1>",
            ReadWorkspaceFileBlocking(dir.GetPath(),
                                      base::FilePath::FromASCII("index.html")));
}

TEST(LeoWorkspaceContentUITest, ReadReturnsEmptyForMissingFile) {
  base::ScopedTempDir dir;
  ASSERT_TRUE(dir.CreateUniqueTempDir());

  EXPECT_EQ("", ReadWorkspaceFileBlocking(
                    dir.GetPath(), base::FilePath::FromASCII("nope.html")));
}

TEST(LeoWorkspaceContentUITest, ReadRefusesToEscapeFolder) {
  base::ScopedTempDir parent;
  ASSERT_TRUE(parent.CreateUniqueTempDir());
  const base::FilePath workspace = parent.GetPath().AppendASCII("workspace");
  ASSERT_TRUE(base::CreateDirectory(workspace));
  ASSERT_TRUE(
      base::WriteFile(parent.GetPath().AppendASCII("secret.txt"), "secret"));

  // Defence in depth: ParseWorkspaceContentPath never emits a parent reference,
  // but the read must refuse one rather than trusting its caller.
  EXPECT_EQ("", ReadWorkspaceFileBlocking(
                    workspace,
                    base::FilePath::FromASCII("..").AppendASCII("secret.txt")));
}

#if !BUILDFLAG(IS_WIN)
TEST(LeoWorkspaceContentUITest, ReadRefusesSymlinkOutOfFolder) {
  base::ScopedTempDir parent;
  ASSERT_TRUE(parent.CreateUniqueTempDir());
  const base::FilePath workspace = parent.GetPath().AppendASCII("workspace");
  ASSERT_TRUE(base::CreateDirectory(workspace));

  const base::FilePath secret = parent.GetPath().AppendASCII("secret.txt");
  ASSERT_TRUE(base::WriteFile(secret, "secret"));
  ASSERT_TRUE(
      base::CreateSymbolicLink(secret, workspace.AppendASCII("link.txt")));

  // The path has no ".." in it, so only the resolved containment check can
  // catch this one.
  EXPECT_EQ("", ReadWorkspaceFileBlocking(
                    workspace, base::FilePath::FromASCII("link.txt")));
}
#endif  // !BUILDFLAG(IS_WIN)

}  // namespace ai_chat
