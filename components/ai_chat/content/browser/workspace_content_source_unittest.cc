// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/content/browser/workspace_content_source.h"

#include <optional>
#include <string>
#include <string_view>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/strings/strcat.h"
#include "brave/components/ai_chat/core/common/constants.h"
#include "build/build_config.h"
#include "content/public/browser/url_data_source.h"
#include "content/public/common/url_constants.h"

#if BUILDFLAG(IS_POSIX)
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace ai_chat {

namespace {

constexpr char kUuid[] = "6b1b3f1e-0b7a-4f27-9a2f-2f2b9d4e5a10";

}  // namespace

// Exercises the real pipeline: a GURL on the leo-workspace host, reduced by
// URLToRequestPath() exactly as WebUIDataSource does before invoking the
// filter, so GURL's canonicalisation of the path is part of what is tested.
class WorkspaceFileRequestPathTest : public testing::Test {
 public:
  void SetUp() override {
    // content::UnitTestTestSuite registers the content schemes; without that
    // the canonicalisation these tests pin would silently stop happening.
    ASSERT_TRUE(GURL(kAIChatLeoWorkspaceUIURL).IsStandard());
  }

 protected:
  // |path| is everything after chrome-untrusted://leo-workspace.
  static std::string RequestPath(std::string_view path) {
    return content::URLDataSource::URLToRequestPath(GURL(base::StrCat(
        {content::kChromeUIUntrustedScheme, url::kStandardSchemeSeparator,
         kAIChatLeoWorkspaceUIHost, path})));
  }

  // |path| is relative to the workspace's files root.
  static std::string FilesRequestPath(std::string_view path) {
    return RequestPath(
        base::StrCat({"/", kUuid, "/", kAIChatLeoWorkspaceFilesSegment, path}));
  }

  base::FilePath ParsedPath(std::string_view path) {
    const std::string request_path = FilesRequestPath(path);
    std::optional<WorkspaceFileRequest> parsed =
        ParseWorkspaceFileRequestPath(request_path);
    EXPECT_TRUE(parsed.has_value()) << request_path;
    if (!parsed) {
      return base::FilePath();
    }
    EXPECT_EQ(kUuid, parsed->uuid);
    return parsed->relative_path;
  }

  void ExpectRejected(std::string_view path) {
    const std::string request_path = FilesRequestPath(path);
    EXPECT_FALSE(ParseWorkspaceFileRequestPath(request_path).has_value())
        << request_path;
  }
};

TEST_F(WorkspaceFileRequestPathTest, ParsesUuidAndRelativePath) {
  EXPECT_EQ(base::FilePath::FromASCII("index.html"), ParsedPath("/index.html"));
  EXPECT_EQ(base::FilePath::FromASCII("assets").AppendASCII("app.js"),
            ParsedPath("/assets/app.js"));
}

TEST_F(WorkspaceFileRequestPathTest, DirectoryRootServesIndex) {
  EXPECT_EQ(base::FilePath::FromASCII("index.html"), ParsedPath(""));
  EXPECT_EQ(base::FilePath::FromASCII("index.html"), ParsedPath("/"));
  EXPECT_EQ(base::FilePath::FromASCII("docs").AppendASCII("index.html"),
            ParsedPath("/docs/"));
}

TEST_F(WorkspaceFileRequestPathTest, StripsQueryAndFragment) {
  EXPECT_EQ(base::FilePath::FromASCII("app.js"), ParsedPath("/app.js?v=2"));
  EXPECT_EQ(base::FilePath::FromASCII("app.js"), ParsedPath("/app.js#top"));
  EXPECT_EQ(base::FilePath::FromASCII("index.html"), ParsedPath("/?v=2"));
}

TEST_F(WorkspaceFileRequestPathTest, DecodesEscapedNames) {
  EXPECT_EQ(base::FilePath::FromUTF8Unsafe("my file.html"),
            ParsedPath("/my%20file.html"));
  // Non-ASCII names are legitimate; they must survive rather than be rejected.
  EXPECT_EQ(base::FilePath::FromUTF8Unsafe("caf\u00e9.html"),
            ParsedPath("/caf%C3%A9.html"));
}

TEST_F(WorkspaceFileRequestPathTest, RejectsEncodedSeparatorsWithinASegment) {
  ExpectRejected("/%2e%2e%2fsecret");
  ExpectRejected("/foo%2f..%2fbar");
  ExpectRejected("/foo%5c..%5cbar");
  // Control bytes.
  ExpectRejected("/%00");
  ExpectRejected("/%0a");
}

TEST_F(WorkspaceFileRequestPathTest, CanonicalisationCollapsesDotSegments) {
  // Traversal arrives already resolved and never reaches DecodePathSegment as
  // "..". Pinned so a GURL change surfaces here, not as a traversal bug.
  for (const char* path : {"/./secret", "/%2e/secret", "/a/../secret"}) {
    EXPECT_EQ(base::FilePath::FromASCII("secret"), ParsedPath(path)) << path;
  }
}

TEST_F(WorkspaceFileRequestPathTest,
       TraversalOutOfTheFilesRootIsNotAFileRequest) {
  // Climbing out of the reserved segment stops the request being a file
  // request rather than escaping: "files" is no longer second.
  EXPECT_FALSE(ParseWorkspaceFileRequestPath(FilesRequestPath("/../secret"))
                   .has_value());
  EXPECT_FALSE(
      ShouldHandleWorkspaceFileRequest(FilesRequestPath("/../secret")));
}

TEST_F(WorkspaceFileRequestPathTest, CanonicalisationConvertsBackslashes) {
  // GURL treats "\" as a separator and collapses the dot segments that follow.
  // Pinned because DecodePathSegment's check is only defence in depth while
  // this holds.
  EXPECT_EQ(base::FilePath::FromASCII("foo").AppendASCII("bar"),
            ParsedPath("/foo\\bar"));
  EXPECT_EQ(base::FilePath::FromASCII("bar"), ParsedPath("/foo\\..\\bar"));
}

TEST_F(WorkspaceFileRequestPathTest, RejectsMalformedUuid) {
  EXPECT_FALSE(
      ParseWorkspaceFileRequestPath(RequestPath("/not-a-uuid/files/index.html"))
          .has_value());
  EXPECT_FALSE(
      ParseWorkspaceFileRequestPath(
          RequestPath("/6B1B3F1E-0B7A-4F27-9A2F-2F2B9D4E5A10/files/index.html"))
          .has_value());
}

TEST_F(WorkspaceFileRequestPathTest, MalformedUuidIsStillClaimed) {
  // Declining would hand it to the default resource, which answers with the
  // tool page. It has to be claimed and then fail.
  EXPECT_TRUE(ShouldHandleWorkspaceFileRequest(
      RequestPath("/not-a-uuid/files/index.html")));
}

TEST_F(WorkspaceFileRequestPathTest, LeavesTheToolPageAndItsBundleAlone) {
  for (const char* path : {"/", "", "/leo_workspace.bundle.js"}) {
    EXPECT_FALSE(ShouldHandleWorkspaceFileRequest(RequestPath(path))) << path;
    EXPECT_FALSE(ParseWorkspaceFileRequestPath(RequestPath(path)).has_value())
        << path;
  }

  const std::string tool_page = RequestPath(base::StrCat({"/", kUuid}));
  EXPECT_FALSE(ShouldHandleWorkspaceFileRequest(tool_page));
  EXPECT_FALSE(ParseWorkspaceFileRequestPath(tool_page).has_value());
}

TEST_F(WorkspaceFileRequestPathTest, RequiresTheReservedSegmentSecond) {
  const std::string nested =
      RequestPath(base::StrCat({"/", kUuid, "/other/files/index.html"}));
  EXPECT_FALSE(ShouldHandleWorkspaceFileRequest(nested));
}

#if BUILDFLAG(IS_WIN)
TEST_F(WorkspaceFileRequestPathTest,
       RejectsWindowsDriveAndAlternateDataStreams) {
  ExpectRejected("/C:");
  ExpectRejected("/C%3a/secret");
  ExpectRejected("/notes.txt:ads");
}
#endif  // BUILDFLAG(IS_WIN)

class WorkspaceContentFileTest : public testing::Test {
 public:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    // Resolved paths come back realpath()-ed, and on macOS the temp dir sits
    // under /var, a symlink to /private/var. Normalise up front.
    root_ = base::MakeAbsoluteFilePath(temp_dir_.GetPath());
    ASSERT_FALSE(root_.empty());
    folder_ = root_.AppendASCII("workspace");
    ASSERT_TRUE(base::CreateDirectory(folder_));
    ASSERT_TRUE(
        base::WriteFile(folder_.AppendASCII("index.html"), "<h1>hi</h1>"));
  }

 protected:
  base::ScopedTempDir temp_dir_;
  base::FilePath root_;
  base::FilePath folder_;
};

TEST_F(WorkspaceContentFileTest, ResolvesFileInsideFolder) {
  EXPECT_EQ(folder_.AppendASCII("index.html"),
            ResolveWorkspaceFileBlocking(
                folder_, base::FilePath::FromASCII("index.html")));
}

TEST_F(WorkspaceContentFileTest, RejectsMissingFile) {
  EXPECT_EQ(std::nullopt, ResolveWorkspaceFileBlocking(
                              folder_, base::FilePath::FromASCII("nope.html")));
}

TEST_F(WorkspaceContentFileTest, RejectsDirectory) {
  ASSERT_TRUE(base::CreateDirectory(folder_.AppendASCII("assets")));
  EXPECT_EQ(std::nullopt, ResolveWorkspaceFileBlocking(
                              folder_, base::FilePath::FromASCII("assets")));
}

TEST_F(WorkspaceContentFileTest, RejectsParentReferences) {
  ASSERT_TRUE(base::WriteFile(root_.AppendASCII("outside.txt"), "secret"));
  EXPECT_EQ(
      std::nullopt,
      ResolveWorkspaceFileBlocking(
          folder_, base::FilePath::FromASCII("..").AppendASCII("outside.txt")));
}

TEST_F(WorkspaceContentFileTest, RejectsAbsolutePaths) {
  base::FilePath outside = root_.AppendASCII("outside.txt");
  ASSERT_TRUE(base::WriteFile(outside, "secret"));
  EXPECT_EQ(std::nullopt, ResolveWorkspaceFileBlocking(folder_, outside));
}

#if BUILDFLAG(IS_POSIX)
TEST_F(WorkspaceContentFileTest, RejectsSymlinkEscapingFolder) {
  base::FilePath outside = root_.AppendASCII("outside.txt");
  ASSERT_TRUE(base::WriteFile(outside, "secret"));
  ASSERT_TRUE(
      base::CreateSymbolicLink(outside, folder_.AppendASCII("link.txt")));

  EXPECT_EQ(std::nullopt, ResolveWorkspaceFileBlocking(
                              folder_, base::FilePath::FromASCII("link.txt")));
}

TEST_F(WorkspaceContentFileTest, AllowsSymlinkWithinFolder) {
  base::FilePath inside = folder_.AppendASCII("real.txt");
  ASSERT_TRUE(base::WriteFile(inside, "fine"));
  ASSERT_TRUE(
      base::CreateSymbolicLink(inside, folder_.AppendASCII("alias.txt")));

  EXPECT_EQ(inside, ResolveWorkspaceFileBlocking(
                        folder_, base::FilePath::FromASCII("alias.txt")));
}

TEST_F(WorkspaceContentFileTest, RejectsFifo) {
  base::FilePath fifo = folder_.AppendASCII("pipe");
  ASSERT_EQ(0, mkfifo(fifo.value().c_str(), 0600));
  EXPECT_EQ(std::nullopt, ResolveWorkspaceFileBlocking(
                              folder_, base::FilePath::FromASCII("pipe")));
}

TEST_F(WorkspaceContentFileTest, RejectsUnreadableFile) {
  base::FilePath unreadable = folder_.AppendASCII("locked.html");
  ASSERT_TRUE(base::WriteFile(unreadable, "secret"));
  ASSERT_TRUE(base::SetPosixFilePermissions(unreadable, 0));

  EXPECT_EQ(std::nullopt,
            ResolveWorkspaceFileBlocking(
                folder_, base::FilePath::FromASCII("locked.html")));
}
#endif  // BUILDFLAG(IS_POSIX)

}  // namespace ai_chat
