// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/tor/tor_profile_manager.h"
#include "brave/browser/tor/tor_profile_service_factory.h"
#include "brave/common/brave_paths.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "brave/components/tor/mock_tor_launcher_factory.h"
#include "brave/components/tor/tor_constants.h"
#include "brave/components/tor/tor_profile_service.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "components/bookmarks/test/bookmark_test_helpers.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "content/public/test/browser_test.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/extensions/extension_service.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_util.h"
#include "extensions/common/extension_id.h"
#endif

#if BUILDFLAG(IPFS_ENABLED)
#include "brave/browser/ipfs/ipfs_service_factory.h"
#endif

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
                            TorLauncherFactory* factory) {
  base::RunLoop run_loop;
  TorProfileManager::SwitchToTorProfile(
      parent_profile,
      base::Bind(&OnUnblockOnProfileCreation, &run_loop, factory));
  run_loop.Run();

  BrowserList* browser_list = BrowserList::GetInstance();
  EXPECT_EQ(2U, browser_list->size());
  return browser_list->get(1)->profile();
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
};

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
#if BUILDFLAG(IPFS_ENABLED)
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
  Profile* tor_profile =
      SwitchToTorProfile(parent_profile, GetTorLauncherFactory());
  ASSERT_TRUE(tor_profile->IsTor());
  EXPECT_TRUE(tor_profile->IsOffTheRecord());
  EXPECT_EQ(tor_profile->GetOriginalProfile(), parent_profile);

  testing::Mock::AllowLeak(GetTorLauncherFactory());
  EXPECT_CALL(*GetTorLauncherFactory(), KillTorProcess).Times(1);
  TorProfileManager::CloseTorProfileWindows(tor_profile);
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

  // TODO(darkdh): Try to resolve GetAllRelatedProfiles caused DCHECK failed.
  // See https://github.com/brave/brave-browser/issues/14902
#if 0
  // "not_allowed" mode will also disable extension in Tor
  const extensions::Extension* incognito_not_allowed_ext =
      InstallExtension(incognito_not_allowed_ext_path(), 1);
  const std::string incognito_not_allowed_id = incognito_not_allowed_ext->id();
  parent_extension_prefs->SetIsIncognitoEnabled(incognito_not_allowed_id, true);
  Profile* primary_otr_profile = parent_profile->GetPrimaryOTRProfile();
  EXPECT_FALSE(extensions::util::IsIncognitoEnabled(incognito_not_allowed_id,
                                                    primary_otr_profile));
  EXPECT_FALSE(extensions::util::IsIncognitoEnabled(incognito_not_allowed_id,
                                                    tor_profile));
#endif
}
#endif
