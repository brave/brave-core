/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/test_util.h"

#include "base/files/file_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_sync/brave_sync_service_factory.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/test/test_bookmark_client.h"
#include "components/sync_preferences/pref_service_mock_factory.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"

namespace brave_sync {

MockBraveSyncClient::MockBraveSyncClient() {}

MockBraveSyncClient::~MockBraveSyncClient() {}

std::unique_ptr<Profile> CreateBraveSyncProfile(const base::FilePath& path) {
  BraveSyncServiceFactory::GetInstance();

  sync_preferences::PrefServiceMockFactory factory;
  auto registry = base::MakeRefCounted<user_prefs::PrefRegistrySyncable>();
  std::unique_ptr<sync_preferences::PrefServiceSyncable> prefs(
      factory.CreateSyncable(registry.get()));
  RegisterUserProfilePrefs(registry.get());

  TestingProfile::Builder profile_builder;
  profile_builder.SetPrefService(std::move(prefs));
  profile_builder.SetPath(path);
  return profile_builder.Build();
}

std::unique_ptr<KeyedService> BuildFakeBookmarkModelForTests(
    content::BrowserContext* context) {
  // Don't need context, unless we have more than one profile
  using namespace bookmarks;
  std::unique_ptr<TestBookmarkClient> client(new TestBookmarkClient());
  BookmarkPermanentNodeList extra_nodes;
  extern int64_t deleted_node_id;
  deleted_node_id = 0xDE1E7ED40DE;

  auto node = std::make_unique<BookmarkPermanentNode>(deleted_node_id);
  node->set_type(bookmarks::BookmarkNode::FOLDER);
  node->set_visible(false);
  node->SetTitle(base::UTF8ToUTF16("Deleted Bookmarks"));

  extra_nodes.push_back(std::move(node));
  client->SetExtraNodesToLoad(std::move(extra_nodes));
  std::unique_ptr<BookmarkModel> model(
      TestBookmarkClient::CreateModelWithClient(std::move(client)));
  return model;
}

void InvalidateBookmarkChangeProcessor() {
  extern bookmarks::BookmarkNode* deleted_node_root;
  DCHECK_NE(deleted_node_root, nullptr);
  deleted_node_root = nullptr;
}

}  // namespace
