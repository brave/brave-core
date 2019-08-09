/* Copyright 2018 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_CLIENT_BOOKMARK_CHANGE_PROCESSOR_H_
#define BRAVE_COMPONENTS_BRAVE_SYNC_CLIENT_BOOKMARK_CHANGE_PROCESSOR_H_

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/time/time.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "brave/components/brave_sync/client/brave_sync_client.h"
#include "brave/components/brave_sync/model/change_processor.h"
#include "components/bookmarks/browser/bookmark_model_observer.h"
#include "components/bookmarks/browser/bookmark_node.h"
#include "components/bookmarks/browser/bookmark_node_data.h"

FORWARD_DECLARE_TEST(BraveBookmarkChangeProcessorTest, IgnoreRapidCreateDelete);
FORWARD_DECLARE_TEST(BraveBookmarkChangeProcessorTest,
    MigrateOrdersForPermanentNodes);
FORWARD_DECLARE_TEST(BraveBookmarkChangeProcessorTest, ExponentialResend);

class BraveBookmarkChangeProcessorTest;

namespace brave_sync {

class BookmarkChangeProcessor : public ChangeProcessor,
                                       bookmarks::BookmarkModelObserver  {
 public:
  static BookmarkChangeProcessor* Create(
      Profile* profile,
      BraveSyncClient* sync_client,
      prefs::Prefs* sync_prefs);
  ~BookmarkChangeProcessor() override;

  // ChangeProcessor implementation
  void Start() override;
  void Stop() override;
  void Reset(bool clear_meta_info) override;
  void ApplyChangesFromSyncModel(const RecordsList &records) override;
  void GetAllSyncData(
      const std::vector<std::unique_ptr<jslib::SyncRecord>>& records,
      SyncRecordAndExistingList* records_and_existing_objects) override;
  void SendUnsynced() override;
  void InitialSync() override;

  void ApplyOrder(const std::string& object_id, const std::string& order);

 private:
  friend class ::BraveBookmarkChangeProcessorTest;
  FRIEND_TEST_ALL_PREFIXES(::BraveBookmarkChangeProcessorTest,
                                                IgnoreRapidCreateDelete);
  FRIEND_TEST_ALL_PREFIXES(::BraveBookmarkChangeProcessorTest,
                                                MigrateOrdersForPermanentNodes);
  FRIEND_TEST_ALL_PREFIXES(::BraveBookmarkChangeProcessorTest,
                                                ExponentialResend);

  BookmarkChangeProcessor(Profile* profile,
                          BraveSyncClient* sync_client,
                          prefs::Prefs* sync_prefs);

  // bookmarks::BookmarkModelObserver:
  void BookmarkModelLoaded(bookmarks::BookmarkModel* model,
                           bool ids_reassigned) override;
  void BookmarkModelBeingDeleted(bookmarks::BookmarkModel* model) override;
  void BookmarkNodeMoved(bookmarks::BookmarkModel* model,
                         const bookmarks::BookmarkNode* old_parent,
                         size_t old_index,
                         const bookmarks::BookmarkNode* new_parent,
                         size_t new_index) override;
  void BookmarkNodeAdded(bookmarks::BookmarkModel* model,
                         const bookmarks::BookmarkNode* parent,
                         size_t index) override;
  void OnWillRemoveBookmarks(bookmarks::BookmarkModel* model,
                             const bookmarks::BookmarkNode* parent,
                             size_t old_index,
                             const bookmarks::BookmarkNode* node) override;
  void BookmarkNodeRemoved(bookmarks::BookmarkModel* model,
                           const bookmarks::BookmarkNode* parent,
                           size_t old_index,
                           const bookmarks::BookmarkNode* node,
                           const std::set<GURL>& no_longer_bookmarked) override;
  void BookmarkAllUserNodesRemoved(bookmarks::BookmarkModel* model,
                                   const std::set<GURL>& removed_urls) override;
  void BookmarkNodeChanged(bookmarks::BookmarkModel* model,
                           const bookmarks::BookmarkNode* node) override;
  void BookmarkMetaInfoChanged(bookmarks::BookmarkModel* model,
                               const bookmarks::BookmarkNode* node) override;
  void BookmarkNodeFaviconChanged(bookmarks::BookmarkModel* model,
                                  const bookmarks::BookmarkNode* node) override;
  void BookmarkNodeChildrenReordered(
      bookmarks::BookmarkModel* model,
      const bookmarks::BookmarkNode* node) override;

  std::unique_ptr<jslib::SyncRecord> BookmarkNodeToSyncBookmark(
      const bookmarks::BookmarkNode* node);
  bookmarks::BookmarkNode* GetDeletedNodeRoot();
  bookmarks::BookmarkNode* GetPendingNodeRoot();
  void CloneBookmarkNodeForDeleteImpl(
      const bookmarks::BookmarkNodeData::Element& element,
      bookmarks::BookmarkNode* parent,
      int index);
  void CloneBookmarkNodeForDelete(
      const std::vector<bookmarks::BookmarkNodeData::Element>& elements,
      bookmarks::BookmarkNode* parent,
      int index);
  // BookmarkModel::Remove will remove parent but put its children under
  // "Other Bookmarks" so we need to explicitly delete children
  void DeleteSelfAndChildren(const bookmarks::BookmarkNode* node);

  void CompletePendingNodesMove(
      const bookmarks::BookmarkNode* created_folder_node,
      const std::string& created_folder_object_id);

  void MigrateOrders();
  void MigrateOrdersForPermanentNode(bookmarks::BookmarkNode* perm_node);
  int GetPermanentNodeIndex(const bookmarks::BookmarkNode* node) const;
  static int FindMigrateSubOrderLength(const std::string& order);

  static base::TimeDelta GetRetryExponentialWaitAmount(int retry_number);
  static void SetCurrentRetryNumber(bookmarks::BookmarkModel* model,
      const bookmarks::BookmarkNode* node, int retry_number);
  static std::vector<int> GetExponentialWaitsForTests();

  static const std::vector<int> kExponentialWaits;
  static const int kMaxSendRetries;

  BraveSyncClient* sync_client_;  // not owned
  prefs::Prefs* sync_prefs_;  // not owned
  Profile* profile_;  // not owned
  bookmarks::BookmarkModel* bookmark_model_;  // not owned

  std::unique_ptr<bookmarks::BookmarkNode> deleted_node_root_;
  std::unique_ptr<bookmarks::BookmarkNode> pending_node_root_;

  DISALLOW_COPY_AND_ASSIGN(BookmarkChangeProcessor);
};

}  // namespace brave_sync

#endif  // BRAVE_COMPONENTS_BRAVE_SYNC_CLIENT_BOOKMARK_CHANGE_PROCESSOR_H_
