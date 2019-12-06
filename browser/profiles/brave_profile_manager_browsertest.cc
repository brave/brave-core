// Copyright 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include <vector>

#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/extensions/brave_tor_client_updater.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/tor/buildflags.h"
#include "brave/common/brave_paths.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "components/bookmarks/test/bookmark_test_helpers.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/test_utils.h"

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/tor/tor_launcher_factory.h"
#endif

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "chrome/browser/extensions/extension_browsertest.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_util.h"
#include "extensions/common/extension_id.h"
#endif

namespace {

// An observer that returns back to test code after a new profile is
// initialized.
void OnUnblockOnProfileCreation(base::RunLoop* run_loop,
                                Profile* profile,
                                Profile::CreateStatus status) {
  if (status == Profile::CREATE_STATUS_INITIALIZED)
    run_loop->Quit();
}

struct TestProfileData {
  base::string16 profile_name;
  base::string16 profile_name_expected_after_migration;
  bool force_default_name;
  base::FilePath profile_path;
};

std::vector<TestProfileData> GetTestProfileData(
    ProfileManager* profile_manager) {
  const std::vector<TestProfileData> profile_data = {
    {
      base::ASCIIToUTF16("Person 1"),
      base::ASCIIToUTF16("Profile 1"), true,
      profile_manager->user_data_dir().Append(
          profile_manager->GetInitialProfileDir())},
    {
      base::ASCIIToUTF16("Person 2"),
      base::ASCIIToUTF16("Profile 2"), true,
      profile_manager->user_data_dir().Append(
          FILE_PATH_LITERAL("testprofile2"))},
    {
      base::ASCIIToUTF16("ZZCustom 3"),
      base::ASCIIToUTF16("ZZCustom 3"), false,
      profile_manager->user_data_dir().Append(
          FILE_PATH_LITERAL("testprofile3"))},
  };
  return profile_data;
}

#if BUILDFLAG(ENABLE_TOR)
Profile* SwitchToTorProfile() {
  base::RunLoop run_loop;
  profiles::SwitchToTorProfile(
      base::Bind(&OnUnblockOnProfileCreation, &run_loop));
  run_loop.Run();

  BrowserList* browser_list = BrowserList::GetInstance();
  EXPECT_EQ(2U, browser_list->size());
  return browser_list->get(1)->profile();
}
#endif

}  // namespace

class BraveProfileManagerTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
#if BUILDFLAG(ENABLE_TOR)
    g_brave_browser_process->tor_client_updater()->SetExecutablePath(
        base::FilePath(FILE_PATH_LITERAL("test")));
#endif
  }

  void SetScriptSetting(HostContentSettingsMap* content_settings,
                        const ContentSettingsPattern& primary_pattern,
                        ContentSetting setting) {
    content_settings->SetContentSettingCustomScope(
        primary_pattern, ContentSettingsPattern::Wildcard(),
        ContentSettingsType::JAVASCRIPT, "", setting);
  }

  ContentSetting GetScriptSetting(HostContentSettingsMap* content_settings,
                                  const GURL& primary_url) {
    return content_settings->GetContentSetting(
        primary_url, GURL(), ContentSettingsType::JAVASCRIPT, "");
  }
};

// Test that legacy profile names (Person X) that have
// not been user-modified are automatically renamed
// to brave profile names (Profile X).
IN_PROC_BROWSER_TEST_F(BraveProfileManagerTest, PRE_MigrateProfileNames) {
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  ProfileAttributesStorage& storage =
      profile_manager->GetProfileAttributesStorage();
  auto profile_data = GetTestProfileData(profile_manager);
  // Create profiles with old default name
  // Two profiles with legacy default names, to check rename happens
  // in correct order.
  // One profile with a custom name to check that it is not renamed.
  // First is the existing default profile.
  ProfileAttributesEntry* entry1;
  bool has_entry1 = storage.GetProfileAttributesWithPath(
      profile_data[0].profile_path, &entry1);
  ASSERT_EQ(has_entry1, true);
  entry1->SetLocalProfileName(profile_data[0].profile_name);
  if (profile_data[0].force_default_name) {
    entry1->SetIsUsingDefaultName(true);
  }
  // Rest are generated
  for (size_t i = 0; i != profile_data.size(); i++) {
    base::RunLoop run_loop;
    profile_manager->CreateProfileAsync(
        profile_data[i].profile_path,
        base::Bind(&OnUnblockOnProfileCreation, &run_loop),
        base::string16(), std::string());
    run_loop.Run();
    ProfileAttributesEntry* entry;
    bool has_entry = storage.GetProfileAttributesWithPath(
        profile_data[i].profile_path, &entry);
    ASSERT_EQ(has_entry, true);
    entry->SetLocalProfileName(profile_data[i].profile_name);
    entry->SetIsUsingDefaultName(profile_data[i].force_default_name);
  }
}

IN_PROC_BROWSER_TEST_F(BraveProfileManagerTest, MigrateProfileNames) {
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  ProfileAttributesStorage& storage =
      profile_manager->GetProfileAttributesStorage();
  auto profile_data = GetTestProfileData(profile_manager);
  auto entries =
      storage.GetAllProfilesAttributesSortedByName();
  // Verify we still have the expected number of profiles.
  ASSERT_EQ(entries.size(), profile_data.size());
  // Order of items in entries and profile_data should be the same
  // since we manually ensure profile_data is alphabetical.
  for (size_t i = 0; i != entries.size(); i++) {
    // Verify the names changed
    ASSERT_EQ(entries[i]->GetName(),
        profile_data[i].profile_name_expected_after_migration);
    // Verify the path matches, i.e. it is the same profile that got the number
    // that the profile had before migration, so we're sure that profile numbers
    // aren't re-assigned.
    ASSERT_EQ(entries[i]->GetPath(), profile_data[i].profile_path);
  }
}

#if BUILDFLAG(ENABLE_TOR)
IN_PROC_BROWSER_TEST_F(BraveProfileManagerTest,
                       SwitchToTorProfileShareBookmarks) {
  ScopedTorLaunchPreventerForTest prevent_tor_process;
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  ASSERT_TRUE(profile_manager);
  Profile* parent_profile = ProfileManager::GetActiveUserProfile();

  // Add a bookmark in parent profile.
  const base::string16 title(base::ASCIIToUTF16("Test"));
  const GURL url1("https://www.test1.com");
  bookmarks::BookmarkModel* parent_bookmark_model =
      BookmarkModelFactory::GetForBrowserContext(parent_profile);
  bookmarks::test::WaitForBookmarkModelToLoad(parent_bookmark_model);
  const bookmarks::BookmarkNode* root =
      parent_bookmark_model->bookmark_bar_node();
  const bookmarks::BookmarkNode* new_node1 =
      parent_bookmark_model->AddURL(root, 0, title, url1);

  Profile* tor_profile = SwitchToTorProfile();
  ASSERT_TRUE(brave::IsTorProfile(tor_profile));
  EXPECT_TRUE(tor_profile->IsOffTheRecord());
  EXPECT_EQ(brave::GetParentProfile(tor_profile), parent_profile);

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

IN_PROC_BROWSER_TEST_F(BraveProfileManagerTest,
                       SwitchToTorProfileInheritPrefs) {
  ScopedTorLaunchPreventerForTest prevent_tor_process;

  ProfileManager* profile_manager = g_browser_process->profile_manager();
  ASSERT_TRUE(profile_manager);
  Profile* parent_profile = ProfileManager::GetActiveUserProfile();

  // Set ShowBookmarkBar preference in the parent profile.
  PrefService* parent_prefs = parent_profile->GetPrefs();
  parent_prefs->SetBoolean(bookmarks::prefs::kShowBookmarkBar, true);
  EXPECT_TRUE(parent_prefs->GetBoolean(bookmarks::prefs::kShowBookmarkBar));

  Profile* tor_profile = SwitchToTorProfile();
  ASSERT_TRUE(brave::IsTorProfile(tor_profile));
  EXPECT_TRUE(tor_profile->IsOffTheRecord());
  EXPECT_EQ(brave::GetParentProfile(tor_profile), parent_profile);

  // Check if ShowBookmarkBar preference is the same as Tor's parent profile.
  PrefService* tor_prefs = tor_profile->GetPrefs();
  EXPECT_TRUE(tor_prefs->GetBoolean(bookmarks::prefs::kShowBookmarkBar));

  // Change ShowBookmarkBar pref in parent profile will reflect in Tor profile.
  parent_prefs->SetBoolean(bookmarks::prefs::kShowBookmarkBar, false);
  EXPECT_FALSE(tor_prefs->GetBoolean(bookmarks::prefs::kShowBookmarkBar));
}

IN_PROC_BROWSER_TEST_F(BraveProfileManagerTest,
                       SwitchToTorProfileInheritContentSettings) {
  const GURL brave_url("https://www.brave.com");
  ScopedTorLaunchPreventerForTest prevent_tor_process;
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  ASSERT_TRUE(profile_manager);

  Profile* parent_profile = ProfileManager::GetActiveUserProfile();

  HostContentSettingsMap* parent_content_settings =
      HostContentSettingsMapFactory::GetForProfile(parent_profile);
  SetScriptSetting(parent_content_settings, ContentSettingsPattern::Wildcard(),
                   CONTENT_SETTING_BLOCK);

  Profile* tor_profile = SwitchToTorProfile();
  ASSERT_TRUE(brave::IsTorProfile(tor_profile));
  EXPECT_TRUE(tor_profile->IsOffTheRecord());
  EXPECT_EQ(brave::GetParentProfile(tor_profile), parent_profile);

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

#if BUILDFLAG(ENABLE_EXTENSIONS)
class BraveProfileManagerExtensionTest
    : public extensions::ExtensionBrowserTest {
 public:
  void SetUpOnMainThread() override {
    extensions::ExtensionBrowserTest::SetUpOnMainThread();
    g_brave_browser_process->tor_client_updater()->SetExecutablePath(
        base::FilePath(FILE_PATH_LITERAL("test")));

    // Override extension data dir.
    brave::RegisterPathProvider();
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir_);
  }

  const extensions::Extension* GetExtension(Profile* profile) {
    extensions::ExtensionRegistry* registry =
        extensions::ExtensionRegistry::Get(profile);
    for (const scoped_refptr<const extensions::Extension>& extension :
         registry->enabled_extensions()) {
      if (extension->name() == "Trivial Test Extension")
        return extension.get();
    }
    NOTREACHED();
    return NULL;
  }
};

IN_PROC_BROWSER_TEST_F(BraveProfileManagerExtensionTest,
                       PRE_SwitchToTorProfileDisableExtensions) {
  ScopedTorLaunchPreventerForTest prevent_tor_process;
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  ASSERT_TRUE(profile_manager);
  Profile* parent_profile = ProfileManager::GetActiveUserProfile();
  ASSERT_TRUE(parent_profile);

  // Install an extension in parent profile and enable in incognito.
  const extensions::Extension* extension =
      InstallExtension(test_data_dir_.AppendASCII("trivial_extension"), 1);
  const std::string id = extension->id();
  extensions::ExtensionPrefs* parent_extension_prefs =
      extensions::ExtensionPrefs::Get(parent_profile);
  parent_extension_prefs->SetIsIncognitoEnabled(id, true);
}

IN_PROC_BROWSER_TEST_F(BraveProfileManagerExtensionTest,
                       SwitchToTorProfileDisableExtensions) {
  ScopedTorLaunchPreventerForTest prevent_tor_process;
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  ASSERT_TRUE(profile_manager);
  Profile* parent_profile = ProfileManager::GetActiveUserProfile();
  ASSERT_TRUE(parent_profile);

  Profile* tor_profile = SwitchToTorProfile();
  ASSERT_TRUE(brave::IsTorProfile(tor_profile));
  EXPECT_TRUE(tor_profile->IsOffTheRecord());
  EXPECT_EQ(brave::GetParentProfile(tor_profile), parent_profile);

  const extensions::Extension* extension = GetExtension(parent_profile);
  const std::string id = extension->id();

  // The installed extension should not be accessible in Tor.
  EXPECT_TRUE(extensions::util::IsIncognitoEnabled(id, parent_profile));
  extensions::ExtensionRegistry* tor_registry =
      extensions::ExtensionRegistry::Get(tor_profile);
  EXPECT_FALSE(tor_registry->GetExtensionById(
      id, extensions::ExtensionRegistry::EVERYTHING));
}
#endif
#endif
