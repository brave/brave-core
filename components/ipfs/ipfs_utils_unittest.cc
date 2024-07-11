/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipfs_utils.h"

#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/browser/sync/sync_service_factory.h"
#include "chrome/test/base/testing_profile.h"
#include "components/sync_preferences/pref_service_mock_factory.h"
#include "content/public/test/browser_task_environment.h"
#include "net/base/url_util.h"
#include "testing/gtest/include/gtest/gtest.h"

#if !BUILDFLAG(IS_IOS) && !BUILDFLAG(IS_ANDROID)
#include "base/path_service.h"
#include "brave/components/ipfs/ipfs_component_cleaner.h"
#include "chrome/common/chrome_paths.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#endif  // !BUILDFLAG(IS_IOS) && !BUILDFLAG(IS_ANDROID)
namespace {
#if !BUILDFLAG(IS_IOS) && !BUILDFLAG(IS_ANDROID)
// Simple function to dump some text into a new file.
void CreateTextFile(const base::FilePath& filename,
                    const std::wstring& contents) {
  std::wofstream file;
#if BUILDFLAG(IS_WIN)
  file.open(filename.value().c_str());
#elif BUILDFLAG(IS_POSIX) || BUILDFLAG(IS_FUCHSIA)
  file.open(filename.value());
#endif  // BUILDFLAG(IS_WIN)
  ASSERT_TRUE(file.is_open());
  file << contents;
  file.close();
}
#endif  // !BUILDFLAG(IS_IOS) && !BUILDFLAG(IS_ANDROID)

GURL GetDefaultIPFSGateway() {
  return GURL(ipfs::kDefaultPublicGateway);
}
}  // namespace

class IpfsUtilsUnitTest : public testing::Test {
 public:
  IpfsUtilsUnitTest() = default;
  ~IpfsUtilsUnitTest() override = default;

 protected:
  void SetUp() override {
#if !BUILDFLAG(IS_IOS) && !BUILDFLAG(IS_ANDROID)
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    base::FilePath user_data_dir_tmp =
        temp_dir_.GetPath().Append(FILE_PATH_LITERAL("user_data"));
    ASSERT_TRUE(
        base::PathService::Override(chrome::DIR_USER_DATA, user_data_dir_tmp));
    base::FilePath user_data_dir;
    ASSERT_TRUE(base::PathService::Get(chrome::DIR_USER_DATA, &user_data_dir));
    user_data_path_ = user_data_dir;
#endif  // !BUILDFLAG(IS_IOS) && !BUILDFLAG(IS_ANDROID)
  }

  content::BrowserTaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
#if !BUILDFLAG(IS_IOS) && !BUILDFLAG(IS_ANDROID)
  base::ScopedTempDir temp_dir_;
  base::FilePath user_data_path_;
#endif  // !BUILDFLAG(IS_IOS) && !BUILDFLAG(IS_ANDROID)
};

#if !BUILDFLAG(IS_IOS) && !BUILDFLAG(IS_ANDROID)
TEST_F(IpfsUtilsUnitTest, DeleteIpfsComponentAndDataTest) {
  EXPECT_FALSE(user_data_path_.empty());
  base::FilePath cache_folder =
      user_data_path_.Append(FILE_PATH_LITERAL("brave_ipfs"));
  base::CreateDirectory(cache_folder);
  EXPECT_TRUE(base::PathExists(cache_folder));
  base::FilePath cache_folder_subdir =
      cache_folder.Append(FILE_PATH_LITERAL("subdir1"));
  base::CreateDirectory(cache_folder_subdir);
  EXPECT_TRUE(base::PathExists(cache_folder_subdir));
  base::FilePath cache_folder_subdir_file_01 =
      cache_folder_subdir.Append(FILE_PATH_LITERAL("The file 01.txt"));
  CreateTextFile(cache_folder_subdir_file_01, L"12345678901234567890");

  base::FilePath component_id_folder =
      user_data_path_.Append(base::FilePath(ipfs::GetIpfsClientComponentId()));
  base::CreateDirectory(component_id_folder);
  EXPECT_TRUE(base::PathExists(component_id_folder));
  base::FilePath component_id_folde_subdir =
      component_id_folder.Append(FILE_PATH_LITERAL("subdir1"));
  base::CreateDirectory(component_id_folde_subdir);
  EXPECT_TRUE(base::PathExists(component_id_folde_subdir));
  base::FilePath component_id_folde_subdir_file_01 =
      component_id_folde_subdir.Append(FILE_PATH_LITERAL("The file 01.txt"));
  CreateTextFile(component_id_folde_subdir_file_01, L"12345678901234567890");

  ipfs::DeleteIpfsComponent(ipfs::GetIpfsClientComponentPath());
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(base::PathExists(cache_folder));
  EXPECT_FALSE(base::PathExists(component_id_folder));
}
#endif  // !BUILDFLAG(IS_IOS) && !BUILDFLAG(IS_ANDROID)

TEST_F(IpfsUtilsUnitTest, TranslateIPFSURINotIPFSScheme) {
  GURL url(
      base::StrCat({GetDefaultIPFSGateway().spec(),
                    "/ipfs/QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG"}));
  GURL new_url;
  ASSERT_FALSE(ipfs::TranslateIPFSURI(url, &new_url, false));
}

TEST_F(IpfsUtilsUnitTest, TranslateIPFSURIIPFSScheme) {
  GURL url("ipfs://QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, false));
  EXPECT_EQ(new_url,
            GURL(base::StrCat(
                {GetDefaultIPFSGateway().spec(),
                 "ipfs/QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG"})));
}

TEST_F(IpfsUtilsUnitTest, TranslateIPFSURIIPNSScheme) {
  GURL url("ipns://QmSrPmbaUKA3ZodhzPWZnpFgcPMFWF4QsxXbkWfEptTBJd");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, false));
  EXPECT_EQ(new_url,
            GURL(base::StrCat(
                {GetDefaultIPFSGateway().spec(),
                 "ipns/QmSrPmbaUKA3ZodhzPWZnpFgcPMFWF4QsxXbkWfEptTBJd"})));
}

TEST_F(IpfsUtilsUnitTest, RFC3986TranslateIPFSURIIPFSSchemePublic) {
  GURL url("ipfs:QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, false));
  EXPECT_EQ(new_url,
            GURL(base::StrCat(
                {GetDefaultIPFSGateway().spec(),
                 "ipfs/QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG"})));
}

TEST_F(IpfsUtilsUnitTest, RFC3986TranslateIPFSURIIPNSSchemePublic) {
  GURL url("ipns:QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, false));
  EXPECT_EQ(new_url,
            GURL(base::StrCat(
                {GetDefaultIPFSGateway().spec(),
                 "ipns/QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG"})));
}

TEST_F(IpfsUtilsUnitTest, TranslateIPFSURIIPFSSchemeWithPath) {
  GURL url(
      "ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
      "/wiki/Vincent_van_Gogh.html");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, false));
  EXPECT_EQ(
      new_url,
      GURL(base::StrCat(
          {GetDefaultIPFSGateway().spec(),
           "ipfs/bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq",
           "/wiki/Vincent_van_Gogh.html"})));
}

TEST_F(IpfsUtilsUnitTest, TranslateIPFSURIIPFSSchemeWithPathAndHash) {
  GURL url(
      "ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
      "/wiki/Vincent_van_Gogh.html#Emerging_artist");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, false));
  EXPECT_EQ(
      new_url,
      GURL(base::StrCat(
          {GetDefaultIPFSGateway().spec(),
           "ipfs/bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq",
           "/wiki/Vincent_van_Gogh.html#Emerging_artist"})));
}

TEST_F(IpfsUtilsUnitTest, TranslateIPFSURIIPFSSchemeWithPathAndQuery) {
  GURL url(
      "ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
      "/wiki/Vincent_van_Gogh.html?test=true");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, false));
  EXPECT_EQ(
      new_url,
      GURL(base::StrCat(
          {GetDefaultIPFSGateway().spec(),
           "ipfs/bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq",
           "/wiki/Vincent_van_Gogh.html?test=true"})));
}

TEST_F(IpfsUtilsUnitTest, TranslateIPFSURIIPFSSchemeWithPathQueryHash) {
  GURL url(
      "ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
      "/wiki/Vincent_van_Gogh.html?test=true#test");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, false));
  EXPECT_EQ(
      new_url,
      GURL(base::StrCat(
          {GetDefaultIPFSGateway().spec(),
           "ipfs/bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq",
           "/wiki/Vincent_van_Gogh.html?test=true#test"})));
}

TEST_F(IpfsUtilsUnitTest, TranslateIPFSURINotIPFSSchemeSubdomain) {
  GURL url(
      "http://a.com/ipfs/bafybeiffndsajwhk3lwjewwdxqntmjm4b5wxaaanokonsgg"
      "enkbw6slwk4");
  GURL new_url;
  ASSERT_FALSE(ipfs::TranslateIPFSURI(url, &new_url, true));
}

TEST_F(IpfsUtilsUnitTest, TranslateIPFSURIIPFSSchemeSubdomain) {
  GURL url(
      "ipfs://bafybeiffndsajwhk3lwjewwdxqntmjm4b5wxaaanokonsggenkbw6slwk4");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, true));
  EXPECT_EQ(new_url,
            GURL(base::StrCat(
                {"https://bafybeiffndsajwhk3lwjewwdxqntmjm4b5wxaaanokonsggenkb",
                 "w6slwk4.ipfs.", GetDefaultIPFSGateway().host(), "/"})));
}

TEST_F(IpfsUtilsUnitTest, TranslateIPFSURIIPNSSchemeSubdomain) {
  GURL url(
      "ipns://bafybeiffndsajwhk3lwjewwdxqntmjm4b5wxaaanokonsggenkbw6slwk4");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, true));
  EXPECT_EQ(new_url,
            GURL(base::StrCat(
                {"https://bafybeiffndsajwhk3lwjewwdxqntmjm4b5wxaaanokonsggenkb"
                 "w6slwk4.ipns.",
                 GetDefaultIPFSGateway().host(), "/"})));
}

TEST_F(IpfsUtilsUnitTest, TranslateIPFSURIIPFSSchemeWithPathSubdomain) {
  GURL url(
      "ipfs://bafybeiffndsajwhk3lwjewwdxqntmjm4b5wxaaanokonsggenkbw6slwk4"
      "/wiki/Vincent_van_Gogh.html");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, true));
  EXPECT_EQ(new_url,
            GURL(base::StrCat({
                "https://bafybeiffndsajwhk3lwjewwdxqntmjm4b5wxaaanokonsggenkb",
                "w6slwk4.ipfs.",
                GetDefaultIPFSGateway().host(),
                "/wiki/Vincent_van_Gogh.html",
            })));
}

TEST_F(IpfsUtilsUnitTest, TranslateIPFSURIIPFSSchemeWithPathAndHashSubdomain) {
  GURL url(
      "ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
      "/wiki/Vincent_van_Gogh.html#Emerging_artist");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, true));
  EXPECT_EQ(new_url,
            GURL(base::StrCat(
                {"https://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3ev",
                 "fyavhwq.ipfs.", GetDefaultIPFSGateway().host(),
                 "/wiki/Vincent_van_Gogh.html#Emerging_artist"})));
}

TEST_F(IpfsUtilsUnitTest, TranslateIPFSURIIPFSSchemeWithPathAndQuerySubdomain) {
  GURL url(
      "ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
      "/wiki/Vincent_van_Gogh.html?test=true");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, true));
  EXPECT_EQ(
      new_url,
      GURL(base::StrCat(
          {"https://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evf",
           "yavhwq.ipfs.", GetDefaultIPFSGateway().host(),
           "/wiki/Vincent_van_Gogh.html?test=true"})));
}

TEST_F(IpfsUtilsUnitTest,
       TranslateIPFSURIIPFSSchemeWithPathQueryHashSubdomain) {
  GURL url(
      "ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
      "/wiki/Vincent_van_Gogh.html?test=true#test");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, true));
  EXPECT_EQ(new_url,
            GURL(base::StrCat(
                {"https://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3ev",
                 "fyavhwq.ipfs.", GetDefaultIPFSGateway().host(),
                 "/wiki/Vincent_van_Gogh.html?test=true", "#test"})));
}

TEST_F(IpfsUtilsUnitTest, ContentHashToIpfsTest) {
  std::string contenthash =
      "e30101701220f073be187e8e06039796c432a"
      "5bdd6da3f403c2f93fa5d9dbdc5547c7fe0e3bc";
  std::vector<uint8_t> hex;
  base::HexStringToBytes(contenthash, &hex);
  GURL ipfs_url = ipfs::ContentHashToCIDv1URL(hex);
  ASSERT_TRUE(ipfs_url.is_valid());
  EXPECT_EQ(
      ipfs_url.spec(),
      "ipfs://bafybeihqoo7bq7uoaybzpfwegks33vw2h5adyl4t7joz3pofkr6h7yhdxq");

  contenthash =
      "e50101701220f073be187e8e06039796c432a"
      "5bdd6da3f403c2f93fa5d9dbdc5547c7fe0e3bc";
  hex.clear();
  base::HexStringToBytes(contenthash, &hex);
  ipfs_url = ipfs::ContentHashToCIDv1URL(hex);
  ASSERT_TRUE(ipfs_url.is_valid());
  EXPECT_EQ(
      ipfs_url.spec(),
      "ipns://bafybeihqoo7bq7uoaybzpfwegks33vw2h5adyl4t7joz3pofkr6h7yhdxq");
  contenthash =
      "0101701220f073be187e8e06039796c432a"
      "5bdd6da3f403c2f93fa5d9dbdc5547c7fe0e3bc";
  hex.clear();
  base::HexStringToBytes(contenthash, &hex);
  ipfs_url = ipfs::ContentHashToCIDv1URL(hex);
  ASSERT_FALSE(ipfs_url.is_valid());
  EXPECT_EQ(ipfs_url.spec(), "");
}
