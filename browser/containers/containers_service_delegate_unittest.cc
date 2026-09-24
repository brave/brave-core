// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/containers/containers_service_delegate.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/functional/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "brave/components/containers/content/browser/storage_partition_utils.h"
#include "brave/components/containers/core/common/features.h"
#include "chrome/common/buildflags.h"
#include "chrome/test/base/testing_profile.h"
#include "components/sessions/core/mock_tab_restore_service.h"
#include "components/sessions/core/serialized_navigation_entry.h"
#include "components/sessions/core/session_id.h"
#include "components/sessions/core/tab_restore_service_observer.h"
#include "components/sessions/core/tab_restore_types.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_SESSION_SERVICE)
#include "brave/components/containers/content/browser/session_utils.h"
#include "chrome/browser/sessions/session_service.h"
#include "chrome/browser/sessions/session_service_test_helper.h"
#include "chrome/browser/ui/browser.h"
#include "components/sessions/content/content_test_helper.h"
#include "ui/base/mojom/window_show_state.mojom.h"
#include "ui/gfx/geometry/rect.h"
#endif

using testing::_;

namespace containers {

namespace {

sessions::SerializedNavigationEntry MakeNavigationWithStorageKey(
    const std::string& partition_domain,
    const std::string& partition_name) {
  sessions::SerializedNavigationEntry nav;
  nav.set_index(0);
  nav.set_virtual_url(GURL("https://example.com/"));
  nav.set_storage_partition_key({partition_domain, partition_name});
  return nav;
}

// Owns `MockTabRestoreService` plus backing storage for `entries()` and
// observer notification used by `ContainersServiceDelegate`.
class TabRestoreTestHarness {
 public:
  TabRestoreTestHarness() {
    ON_CALL(mock_, AddObserver(_))
        .WillByDefault([this](sessions::TabRestoreServiceObserver* observer) {
          observers_.push_back(observer);
        });
    ON_CALL(mock_, RemoveObserver(_))
        .WillByDefault([this](sessions::TabRestoreServiceObserver* observer) {
          observers_.erase(
              std::remove(observers_.begin(), observers_.end(), observer),
              observers_.end());
        });
    ON_CALL(mock_, entries()).WillByDefault(testing::ReturnRef(entries_));
    ON_CALL(mock_, IsLoaded()).WillByDefault([this]() { return loaded_; });
    ON_CALL(mock_, LoadTabsFromLastSession()).WillByDefault([]() {});
  }

  MockTabRestoreService* mock() { return &mock_; }

  void AddEntry(std::unique_ptr<sessions::tab_restore::Entry> entry) {
    entries_.push_back(std::move(entry));
  }

  void set_finish_load_asynchronously(bool async) {
    if (!async) {
      ON_CALL(mock_, LoadTabsFromLastSession()).WillByDefault([]() {});
      return;
    }
    loaded_ = false;
    ON_CALL(mock_, LoadTabsFromLastSession()).WillByDefault([this]() {
      base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
          FROM_HERE, base::BindOnce(&TabRestoreTestHarness::CompleteAsyncLoad,
                                    base::Unretained(this)));
    });
  }

 private:
  void CompleteAsyncLoad() {
    loaded_ = true;
    for (sessions::TabRestoreServiceObserver* observer : observers_) {
      observer->TabRestoreServiceLoaded(&mock_);
    }
  }

  testing::NiceMock<MockTabRestoreService> mock_;
  sessions::TabRestoreService::Entries entries_;
  std::vector<sessions::TabRestoreServiceObserver*> observers_;
  bool loaded_ = true;
};

}  // namespace

class ContainersServiceDelegateTest : public testing::Test {
 public:
  ContainersServiceDelegateTest() = default;

 protected:
  void SetUp() override {
    feature_list_.InitAndEnableFeature(features::kContainers);
  }

  base::test::ScopedFeatureList feature_list_;
  content::BrowserTaskEnvironment task_environment_;
  TestingProfile profile_;
  TabRestoreTestHarness tab_restore_;
};

TEST_F(ContainersServiceDelegateTest,
       GetReferencedContainerIds_EmptyWhenNoTabRestoreOrOpenTabs) {
  ContainersServiceDelegate delegate(&profile_,
#if BUILDFLAG(ENABLE_SESSION_SERVICE)
                                     nullptr,
#endif
                                     nullptr);

  base::test::TestFuture<const base::flat_set<std::string>&> future;
  delegate.GetReferencedContainerIds(future.GetCallback());
  EXPECT_TRUE(future.Get().empty());
}

TEST_F(ContainersServiceDelegateTest,
       GetReferencedContainerIds_IncludesIdsFromLoadedTabRestoreTabs) {
  auto tab = std::make_unique<sessions::tab_restore::Tab>();
  tab->id = SessionID::FromSerializedValue(1);
  tab->navigations.push_back(MakeNavigationWithStorageKey(
      kContainersStoragePartitionDomain, "container-a"));
  tab_restore_.AddEntry(std::move(tab));

  ContainersServiceDelegate delegate(&profile_,
#if BUILDFLAG(ENABLE_SESSION_SERVICE)
                                     nullptr,
#endif
                                     tab_restore_.mock());

  base::test::TestFuture<const base::flat_set<std::string>&> future;
  delegate.GetReferencedContainerIds(future.GetCallback());
  EXPECT_THAT(future.Get(), testing::UnorderedElementsAre("container-a"));
}

TEST_F(ContainersServiceDelegateTest,
       GetReferencedContainerIds_IncludesIdsFromWindowEntries) {
  auto inner = std::make_unique<sessions::tab_restore::Tab>();
  inner->id = SessionID::FromSerializedValue(2);
  inner->navigations.push_back(MakeNavigationWithStorageKey(
      kContainersStoragePartitionDomain, "win-tab"));

  auto window = std::make_unique<sessions::tab_restore::Window>();
  window->id = SessionID::FromSerializedValue(3);
  window->tabs.push_back(std::move(inner));
  tab_restore_.AddEntry(std::move(window));

  ContainersServiceDelegate delegate(&profile_,
#if BUILDFLAG(ENABLE_SESSION_SERVICE)
                                     nullptr,
#endif
                                     tab_restore_.mock());

  base::test::TestFuture<const base::flat_set<std::string>&> future;
  delegate.GetReferencedContainerIds(future.GetCallback());
  EXPECT_THAT(future.Get(), testing::UnorderedElementsAre("win-tab"));
}

TEST_F(ContainersServiceDelegateTest,
       GetReferencedContainerIds_IncludesIdsFromGroupEntries) {
  auto inner = std::make_unique<sessions::tab_restore::Tab>();
  inner->id = SessionID::FromSerializedValue(4);
  inner->navigations.push_back(MakeNavigationWithStorageKey(
      kContainersStoragePartitionDomain, "group-tab"));

  auto group = std::make_unique<sessions::tab_restore::Group>();
  group->id = SessionID::FromSerializedValue(5);
  group->tabs.push_back(std::move(inner));
  tab_restore_.AddEntry(std::move(group));

  ContainersServiceDelegate delegate(&profile_,
#if BUILDFLAG(ENABLE_SESSION_SERVICE)
                                     nullptr,
#endif
                                     tab_restore_.mock());

  base::test::TestFuture<const base::flat_set<std::string>&> future;
  delegate.GetReferencedContainerIds(future.GetCallback());
  EXPECT_THAT(future.Get(), testing::UnorderedElementsAre("group-tab"));
}

TEST_F(ContainersServiceDelegateTest,
       GetReferencedContainerIds_IgnoresNonContainerPartitions) {
  auto tab = std::make_unique<sessions::tab_restore::Tab>();
  tab->id = SessionID::FromSerializedValue(6);
  tab->navigations.push_back(
      MakeNavigationWithStorageKey("extensions", "some-extension-id"));
  tab_restore_.AddEntry(std::move(tab));

  ContainersServiceDelegate delegate(&profile_,
#if BUILDFLAG(ENABLE_SESSION_SERVICE)
                                     nullptr,
#endif
                                     tab_restore_.mock());

  base::test::TestFuture<const base::flat_set<std::string>&> future;
  delegate.GetReferencedContainerIds(future.GetCallback());
  EXPECT_TRUE(future.Get().empty());
}

TEST_F(ContainersServiceDelegateTest,
       GetReferencedContainerIds_DefersUntilTabRestoreLoadsAsync) {
  tab_restore_.set_finish_load_asynchronously(true);

  auto tab = std::make_unique<sessions::tab_restore::Tab>();
  tab->id = SessionID::FromSerializedValue(7);
  tab->navigations.push_back(MakeNavigationWithStorageKey(
      kContainersStoragePartitionDomain, "async-c"));
  tab_restore_.AddEntry(std::move(tab));

  ContainersServiceDelegate delegate(&profile_,
#if BUILDFLAG(ENABLE_SESSION_SERVICE)
                                     nullptr,
#endif
                                     tab_restore_.mock());

  base::test::TestFuture<const base::flat_set<std::string>&> future;
  delegate.GetReferencedContainerIds(future.GetCallback());
  EXPECT_THAT(future.Get(), testing::UnorderedElementsAre("async-c"));
}

TEST_F(ContainersServiceDelegateTest,
       DeleteContainerStorage_CallbackTrueWhenPartitionDirectoryAbsent) {
  ContainersServiceDelegate delegate(&profile_,
#if BUILDFLAG(ENABLE_SESSION_SERVICE)
                                     nullptr,
#endif
                                     nullptr);

  base::test::TestFuture<bool> future;
  delegate.DeleteContainerStorage("unused-container-id", future.GetCallback());
  EXPECT_TRUE(future.Get());
}

#if BUILDFLAG(ENABLE_SESSION_SERVICE)

// SessionService-backed tests: owns the service + helper and persists a last
// session to disk (recreate pattern from SessionServiceTest::ReadWindows).
class ContainersServiceDelegateSessionServiceTest
    : public ContainersServiceDelegateTest {
 protected:
  void SetUp() override {
    ContainersServiceDelegateTest::SetUp();
    session_service_ = std::make_unique<SessionService>(&profile_);
    session_helper_.SetService(session_service_.get());
    session_helper_.SetSavingEnabled(true);
  }

  void PersistLastSessionWithOneContainerTab(const std::string& container_id) {
    SessionService* const session = session_service_.get();
    const SessionID window_id = SessionID::NewUnique();
    session->SetWindowType(window_id, Browser::TYPE_NORMAL);
    session->SetWindowBounds(window_id, gfx::Rect(0, 0, 100, 100),
                             ui::mojom::WindowShowState::kNormal);

    const SessionID tab_id = SessionID::NewUnique();
    session_helper_.PrepareTabInWindow(window_id, tab_id, 0, true);
    session->UpdateTabNavigation(window_id, tab_id,
                                 MakePersistedSessionNavigation(container_id));
    session->SetSelectedNavigationIndex(window_id, tab_id, 0);
    session_helper_.SaveNow();

    session_service_ = std::make_unique<SessionService>(&profile_);
    session_helper_.SetService(session_service_.get());
  }

 protected:
  static sessions::SerializedNavigationEntry MakePersistedSessionNavigation(
      const std::string& container_id) {
    sessions::SerializedNavigationEntry nav =
        sessions::ContentTestHelper::CreateNavigation("https://www.google.com/",
                                                      "t");
    nav.set_index(0);
    const std::pair<std::string, std::string> key = {
        kContainersStoragePartitionDomain, container_id};
    nav.set_storage_partition_key(key);
    std::optional<std::string> prefix = StoragePartitionKeyToUrlPrefix(key);
    CHECK(prefix.has_value());
    nav.set_virtual_url_prefix(*prefix);
    return nav;
  }

  std::unique_ptr<SessionService> session_service_;
  SessionServiceTestHelper session_helper_;
};

TEST_F(ContainersServiceDelegateSessionServiceTest,
       GetReferencedContainerIds_IncludesIdsFromSessionServiceLastSession) {
  PersistLastSessionWithOneContainerTab("last-session-c");

  ContainersServiceDelegate delegate(&profile_, session_service_.get(),
                                     tab_restore_.mock());

  base::test::TestFuture<const base::flat_set<std::string>&> future;
  delegate.GetReferencedContainerIds(future.GetCallback());
  EXPECT_THAT(future.Get(), testing::UnorderedElementsAre("last-session-c"));
}

TEST_F(ContainersServiceDelegateSessionServiceTest,
       GetReferencedContainerIds_MergesLastSessionAndTabRestore) {
  PersistLastSessionWithOneContainerTab("from-session");

  auto closed_tab = std::make_unique<sessions::tab_restore::Tab>();
  closed_tab->id = SessionID::FromSerializedValue(99);
  closed_tab->navigations.push_back(MakeNavigationWithStorageKey(
      kContainersStoragePartitionDomain, "from-restore"));
  tab_restore_.AddEntry(std::move(closed_tab));

  ContainersServiceDelegate delegate(&profile_, session_service_.get(),
                                     tab_restore_.mock());

  base::test::TestFuture<const base::flat_set<std::string>&> future;
  delegate.GetReferencedContainerIds(future.GetCallback());
  EXPECT_THAT(future.Get(),
              testing::UnorderedElementsAre("from-session", "from-restore"));
}

#endif  // BUILDFLAG(ENABLE_SESSION_SERVICE)

}  // namespace containers
