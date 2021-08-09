// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "base/path_service.h"
#include "base/process/launch.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/tor/tor_profile_manager.h"
#include "brave/browser/tor/tor_profile_service_factory.h"
#include "brave/common/brave_paths.h"
#include "brave/common/brave_switches.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "brave/components/tor/mock_tor_launcher_factory.h"
#include "brave/components/tor/tor_constants.h"
#include "brave/components/tor/tor_profile_service.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "components/bookmarks/test/bookmark_test_helpers.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "content/public/test/browser_test.h"
#include "extensions/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/extensions/extension_service.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_util.h"
#include "extensions/common/extension_id.h"
#endif

#if BUILDFLAG(ENABLE_IPFS)
#include "brave/browser/ipfs/ipfs_service_factory.h"
#endif

#include <algorithm>

namespace {

// An observer that returns back to test code after a new profile is
// initialized.
void OnUnblockOnProfileCreation(base::RunLoop* run_loop,
                                TorLauncherFactory* factory,
                                Profile* profile,
                                Profile::CreateStatus status) {
  if (status == Profile::CREATE_STATUS_INITIALIZED) {
    tor::TorProfileService* service =
        TorProfileServiceFactory::GetForContext(profile);
    service->SetTorLauncherFactoryForTest(factory);
    run_loop->Quit();
  }
}

Profile* SwitchToTorProfile(Profile* parent_profile,
                            TorLauncherFactory* factory,
                            size_t current_profile_num = 1) {
  base::RunLoop run_loop;
  TorProfileManager::SwitchToTorProfile(
      parent_profile,
      base::BindRepeating(&OnUnblockOnProfileCreation, &run_loop, factory));
  run_loop.Run();

  BrowserList* browser_list = BrowserList::GetInstance();
  EXPECT_EQ(current_profile_num + 1, browser_list->size());
  return browser_list->get(current_profile_num)->profile();
}

}  // namespace

class TorProfileManagerTest : public InProcessBrowserTest {
 public:
  TorProfileManagerTest() = default;

  void SetScriptSetting(HostContentSettingsMap* content_settings,
                        const ContentSettingsPattern& primary_pattern,
                        ContentSetting setting) {
    content_settings->SetContentSettingCustomScope(
        primary_pattern, ContentSettingsPattern::Wildcard(),
        ContentSettingsType::JAVASCRIPT, setting);
  }

  ContentSetting GetScriptSetting(HostContentSettingsMap* content_settings,
                                  const GURL& primary_url) {
    return content_settings->GetContentSetting(primary_url, GURL(),
                                               ContentSettingsType::JAVASCRIPT);
  }

  MockTorLauncherFactory* GetTorLauncherFactory() {
    return &MockTorLauncherFactory::GetInstance();
  }

  void Relaunch(const base::CommandLine& new_command_line) {
    base::LaunchProcess(new_command_line, base::LaunchOptionsForTest());
  }
};

// We don't run this test on Mac because the function GetCommandLineForRelaunch
// isn't defined there.
#if !defined(OS_MAC)
IN_PROC_BROWSER_TEST_F(TorProfileManagerTest, LaunchWithTorUrl) {
  // We should start with one normal window.
  ASSERT_EQ(1u, chrome::GetTabbedBrowserCount(browser()->profile()));

  // Run with --tor switch and a URL specified.
  base::FilePath test_file_path = ui_test_utils::GetTestFilePath(
      base::FilePath(), base::FilePath().AppendASCII("empty.html"));
  base::CommandLine new_command_line(GetCommandLineForRelaunch());
  new_command_line.AppendSwitch(switches::kTor);
  new_command_line.AppendArgPath(test_file_path);

  Relaunch(new_command_line);

  // There should be one normal and one Tor window now.
  Relaunch(new_command_line);
  ui_test_utils::WaitForBrowserToOpen();
  ASSERT_EQ(2u, chrome::GetTotalBrowserCount());
  ASSERT_EQ(1u, chrome::GetTabbedBrowserCount(browser()->profile()));
}
#endif

IN_PROC_BROWSER_TEST_F(TorProfileManagerTest,
                       SwitchToTorProfileShareBookmarks) {
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  ASSERT_TRUE(profile_manager);
  Profile* parent_profile = ProfileManager::GetActiveUserProfile();

  // Add a bookmark in parent profile.
  const std::u16string title(u"Test");
  const GURL url1("https://www.test1.com");
  bookmarks::BookmarkModel* parent_bookmark_model =
      BookmarkModelFactory::GetForBrowserContext(parent_profile);
  bookmarks::test::WaitForBookmarkModelToLoad(parent_bookmark_model);
  const bookmarks::BookmarkNode* root =
      parent_bookmark_model->bookmark_bar_node();
  const bookmarks::BookmarkNode* new_node1 =
      parent_bookmark_model->AddURL(root, 0, title, url1);

  Profile* tor_profile =
      SwitchToTorProfile(parent_profile, GetTorLauncherFactory());
  ASSERT_TRUE(tor_profile->IsTor());
  EXPECT_TRUE(tor_profile->IsOffTheRecord());
  EXPECT_EQ(tor_profile->GetOriginalProfile(), parent_profile);

  // Check if the same node is in Tor profile since we share the bookmark
  // service between Tor profile and its parent profile.
  bookmarks::BookmarkModel* tor_bookmark_model =
      BookmarkModelFactory::GetForBrowserContext(tor_profile);
  bookmarks::test::WaitForBookmarkModelToLoad(tor_bookmark_model);
  EXPECT_EQ(tor_bookmark_model->GetMostRecentlyAddedUserNodeForURL(url1),
            new_node1);

  // Add a new bookmark in parent profile again and check if it would show up
  // in Tor profile.
  const GURL url2("https://www.test2.com");
  const bookmarks::BookmarkNode* new_node2 =
      parent_bookmark_model->AddURL(root, 0, title, url2);
  EXPECT_EQ(tor_bookmark_model->GetMostRecentlyAddedUserNodeForURL(url2),
            new_node2);

  // Add a new bookmark through tor profile and check if it would show up in
  // its parent profile.
  const GURL url3("https://www.test3.com");
  const bookmarks::BookmarkNode* tor_root =
      tor_bookmark_model->bookmark_bar_node();
  const bookmarks::BookmarkNode* new_node3 =
      tor_bookmark_model->AddURL(tor_root, 0, title, url3);
  EXPECT_EQ(parent_bookmark_model->GetMostRecentlyAddedUserNodeForURL(url3),
            new_node3);
}

IN_PROC_BROWSER_TEST_F(TorProfileManagerTest,
                       SwitchToTorProfileExcludeServices) {
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  ASSERT_TRUE(profile_manager);
  Profile* parent_profile = ProfileManager::GetActiveUserProfile();

  Profile* tor_profile =
      SwitchToTorProfile(parent_profile, GetTorLauncherFactory());
  EXPECT_EQ(tor_profile->GetOriginalProfile(), parent_profile);
  ASSERT_TRUE(tor_profile->IsTor());
  EXPECT_TRUE(tor_profile->IsOffTheRecord());

  EXPECT_EQ(brave_rewards::RewardsServiceFactory::GetForProfile(tor_profile),
            nullptr);
  EXPECT_EQ(brave_ads::AdsServiceFactory::GetForProfile(tor_profile), nullptr);
#if BUILDFLAG(ENABLE_IPFS)
  EXPECT_EQ(ipfs::IpfsServiceFactory::GetForContext(tor_profile), nullptr);
#endif
}

IN_PROC_BROWSER_TEST_F(TorProfileManagerTest, SwitchToTorProfileInheritPrefs) {
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  ASSERT_TRUE(profile_manager);
  Profile* parent_profile = ProfileManager::GetActiveUserProfile();

  // Set ShowBookmarkBar preference in the parent profile.
  PrefService* parent_prefs = parent_profile->GetPrefs();
  parent_prefs->SetBoolean(bookmarks::prefs::kShowBookmarkBar, true);
  EXPECT_TRUE(parent_prefs->GetBoolean(bookmarks::prefs::kShowBookmarkBar));

  Profile* tor_profile =
      SwitchToTorProfile(parent_profile, GetTorLauncherFactory());
  ASSERT_TRUE(tor_profile->IsTor());
  EXPECT_TRUE(tor_profile->IsOffTheRecord());
  EXPECT_EQ(tor_profile->GetOriginalProfile(), parent_profile);

  // Check if ShowBookmarkBar preference is the same as Tor's parent profile.
  PrefService* tor_prefs = tor_profile->GetPrefs();
  EXPECT_TRUE(tor_prefs->GetBoolean(bookmarks::prefs::kShowBookmarkBar));

  // Change ShowBookmarkBar pref in parent profile will reflect in Tor profile.
  parent_prefs->SetBoolean(bookmarks::prefs::kShowBookmarkBar, false);
  EXPECT_FALSE(tor_prefs->GetBoolean(bookmarks::prefs::kShowBookmarkBar));
}

IN_PROC_BROWSER_TEST_F(TorProfileManagerTest,
                       SwitchToTorProfileInheritContentSettings) {
  const GURL brave_url("https://www.brave.com");
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  ASSERT_TRUE(profile_manager);

  Profile* parent_profile = ProfileManager::GetActiveUserProfile();

  HostContentSettingsMap* parent_content_settings =
      HostContentSettingsMapFactory::GetForProfile(parent_profile);
  SetScriptSetting(parent_content_settings, ContentSettingsPattern::Wildcard(),
                   CONTENT_SETTING_BLOCK);

  Profile* tor_profile =
      SwitchToTorProfile(parent_profile, GetTorLauncherFactory());
  ASSERT_TRUE(tor_profile->IsTor());
  EXPECT_TRUE(tor_profile->IsOffTheRecord());
  EXPECT_EQ(tor_profile->GetOriginalProfile(), parent_profile);

  // Check Tor profile's content settings are inherited from its parent.
  HostContentSettingsMap* tor_content_settings =
      HostContentSettingsMapFactory::GetForProfile(tor_profile);
  ContentSetting setting = GetScriptSetting(tor_content_settings, brave_url);
  EXPECT_EQ(setting, CONTENT_SETTING_BLOCK);

  // Check changes of content settings from the parent profile will reflected
  // in Tor profile when the setting is not set directly in Tor profile.
  SetScriptSetting(parent_content_settings, ContentSettingsPattern::Wildcard(),
                   CONTENT_SETTING_ALLOW);
  setting = GetScriptSetting(tor_content_settings, brave_url);
  EXPECT_EQ(setting, CONTENT_SETTING_ALLOW);

  // Check changes of content settings from the parent profile will not
  // overwrite the one in Tor profile when it is set directly in Tor profile.
  SetScriptSetting(tor_content_settings,
                   ContentSettingsPattern::FromURL(brave_url),
                   CONTENT_SETTING_BLOCK);
  setting = GetScriptSetting(parent_content_settings, brave_url);
  EXPECT_EQ(setting, CONTENT_SETTING_ALLOW);
  setting = GetScriptSetting(tor_content_settings, brave_url);
  EXPECT_EQ(setting, CONTENT_SETTING_BLOCK);
}

IN_PROC_BROWSER_TEST_F(TorProfileManagerTest, CloseLastTorWindow) {
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  ASSERT_TRUE(profile_manager);

  Profile* parent_profile = ProfileManager::GetActiveUserProfile();
  EXPECT_EQ(BrowserList::GetInstance()->size(), 1u);
  Profile* tor_profile =
      SwitchToTorProfile(parent_profile, GetTorLauncherFactory());
  EXPECT_EQ(BrowserList::GetInstance()->size(), 2u);
  ASSERT_TRUE(tor_profile->IsTor());
  EXPECT_TRUE(tor_profile->IsOffTheRecord());
  EXPECT_EQ(tor_profile->GetOriginalProfile(), parent_profile);

  testing::Mock::AllowLeak(GetTorLauncherFactory());
  EXPECT_CALL(*GetTorLauncherFactory(), KillTorProcess).Times(1);
  TorProfileManager::CloseTorProfileWindows(tor_profile);
  ui_test_utils::WaitForBrowserToClose();
  BrowserList* browser_list = BrowserList::GetInstance();
  ASSERT_EQ(browser_list->size(), 1u);
  EXPECT_FALSE(browser_list->get(0)->profile()->IsTor());
}

IN_PROC_BROWSER_TEST_F(TorProfileManagerTest, CloseAllTorWindows) {
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  ASSERT_TRUE(profile_manager);
  BrowserList* browser_list = BrowserList::GetInstance();

  Profile* parent_profile1 = ProfileManager::GetActiveUserProfile();
  ASSERT_NE(CreateIncognitoBrowser(parent_profile1), nullptr);
  ASSERT_EQ(browser_list->size(), 2u);

  // Create another profile.
  base::FilePath dest_path = profile_manager->user_data_dir();
  dest_path = dest_path.Append(FILE_PATH_LITERAL("Profile2"));
  Profile* parent_profile2 = nullptr;
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    parent_profile2 = profile_manager->GetProfile(dest_path);
  }
  ASSERT_TRUE(parent_profile2);
  ASSERT_NE(CreateBrowser(parent_profile2), nullptr);
  ASSERT_EQ(browser_list->size(), 3u);

  Profile* tor_profile1 = SwitchToTorProfile(
      parent_profile1, GetTorLauncherFactory(), browser_list->size());
  ASSERT_TRUE(tor_profile1->IsTor());
  ASSERT_EQ(browser_list->size(), 4u);

  Profile* tor_profile2 = SwitchToTorProfile(
      parent_profile2, GetTorLauncherFactory(), browser_list->size());
  ASSERT_TRUE(tor_profile2->IsTor());
  ASSERT_EQ(browser_list->size(), 5u);

  testing::Mock::AllowLeak(GetTorLauncherFactory());
  EXPECT_CALL(*GetTorLauncherFactory(), KillTorProcess).Times(1);
  TorProfileManager::GetInstance().CloseAllTorWindows();
  // We cannot predict the order of which Tor browser get closed first
  ui_test_utils::WaitForBrowserToClose();
  ui_test_utils::WaitForBrowserToClose();
  // only two regular windows and one private window left
  ASSERT_EQ(browser_list->size(), 3u);
  std::for_each(
      browser_list->begin(), browser_list->end(),
      [](Browser* browser) { EXPECT_FALSE(browser->profile()->IsTor()); });
}

#if BUILDFLAG(ENABLE_EXTENSIONS)
class TorProfileManagerExtensionTest : public extensions::ExtensionBrowserTest {
 public:
  void SetUpOnMainThread() override {
    extensions::ExtensionBrowserTest::SetUpOnMainThread();

    // Override extension data dir.
    brave::RegisterPathProvider();
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir_);
    extension_path_ = test_data_dir_.AppendASCII("extensions")
                          .AppendASCII("trivial_extension");
    incognito_not_allowed_ext_path_ =
        test_data_dir_.AppendASCII("extensions")
            .AppendASCII("trivial_extension_incognito_not_allowed");
  }

  base::FilePath extension_path() const { return extension_path_; }
  base::FilePath incognito_not_allowed_ext_path() const {
    return incognito_not_allowed_ext_path_;
  }

  TorLauncherFactory* GetTorLauncherFactory() {
    return &MockTorLauncherFactory::GetInstance();
  }

 private:
  base::FilePath extension_path_;
  base::FilePath incognito_not_allowed_ext_path_;
};

IN_PROC_BROWSER_TEST_F(TorProfileManagerExtensionTest,
                       SwitchToTorProfileIncognitoEnabled) {
  Profile* parent_profile = ProfileManager::GetActiveUserProfile();
  ASSERT_TRUE(parent_profile);

  // Install an extension in parent profile and enable in incognito.
  const extensions::Extension* extension =
      InstallExtension(extension_path(), 1);
  const std::string id = extension->id();
  extensions::ExtensionPrefs* parent_extension_prefs =
      extensions::ExtensionPrefs::Get(parent_profile);
  parent_extension_prefs->SetIsIncognitoEnabled(id, true);

  Profile* tor_profile =
      SwitchToTorProfile(parent_profile, GetTorLauncherFactory());
  ASSERT_TRUE(tor_profile->IsTor());
  EXPECT_TRUE(tor_profile->IsOffTheRecord());
  EXPECT_EQ(tor_profile->GetOriginalProfile(), parent_profile);

  // The installed extension should be accessible in Tor.
  EXPECT_TRUE(extensions::util::IsIncognitoEnabled(id, tor_profile));
  EXPECT_TRUE(extensions::util::IsIncognitoEnabled(id, parent_profile));
  // Tor OTR and regular profile shares same registry
  extensions::ExtensionRegistry* parent_registry =
      extensions::ExtensionRegistry::Get(parent_profile);
  extensions::ExtensionRegistry* tor_registry =
      extensions::ExtensionRegistry::Get(tor_profile);
  EXPECT_EQ(parent_registry, tor_registry);
  EXPECT_TRUE(tor_registry->GetExtensionById(
      id, extensions::ExtensionRegistry::EVERYTHING));

  // Component extension should always be allowed
  extension_service()->UnloadExtension(
      extension->id(), extensions::UnloadedExtensionReason::UNINSTALL);
  const extensions::Extension* component_extension =
      LoadExtensionAsComponent(extension_path());
  ASSERT_TRUE(component_extension);
  parent_extension_prefs->SetIsIncognitoEnabled(component_extension->id(),
                                                false);
  EXPECT_TRUE(extensions::util::IsIncognitoEnabled(component_extension->id(),
                                                   tor_profile));
}
#endif
