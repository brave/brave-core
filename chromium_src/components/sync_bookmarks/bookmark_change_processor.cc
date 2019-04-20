namespace bookmarks {
class BookmarkModel;
class BookmarkNode;
}   // namespace bookmarks

namespace sync_bookmarks {
namespace {

void AddBraveMetaInfo(const bookmarks::BookmarkNode* node,
                      bookmarks::BookmarkModel* bookmark_model);

bool IsFirstLoadedFavicon(const bookmarks::BookmarkNode* node);

}   // namespace
}   // namespace sync_bookmarks

#include "../../../../components/sync_bookmarks/bookmark_change_processor.cc"
#include "brave/components/brave_sync/tools.h"
#include "components/bookmarks/browser/bookmark_model.h"
namespace sync_bookmarks {
namespace {

void GetPrevObjectId(const bookmarks::BookmarkNode* parent,
                     int index,
                     std::string* prev_object_id) {
  DCHECK_GE(index, 0);
  auto* prev_node = index == 0 ?
    nullptr :
    parent->GetChild(index - 1);

  if (prev_node)
    prev_node->GetMetaInfo("object_id", prev_object_id);
}

void GetOrder(const bookmarks::BookmarkNode* parent,
              int index,
              std::string* prev_order,
              std::string* next_order,
              std::string* parent_order) {
  DCHECK_GE(index, 0);
  auto* prev_node = index == 0 ?
    nullptr :
    parent->GetChild(index - 1);
  auto* next_node = index == parent->child_count() - 1 ?
    nullptr :
    parent->GetChild(index + 1);

  if (prev_node)
    prev_node->GetMetaInfo("order", prev_order);

  if (next_node)
    next_node->GetMetaInfo("order", next_order);

  parent->GetMetaInfo("order", parent_order);
}

void AddMetaInfo(bookmarks::BookmarkModel* bookmark_model,
                 const bookmarks::BookmarkNode* node,
                 const std::string& key,
                 const std::string& value) {
  bookmark_model->SetNodeMetaInfo(node, key, value);
}

void AddBraveMetaInfo(const bookmarks::BookmarkNode* node,
                      bookmarks::BookmarkModel* bookmark_model) {
  std::string prev_object_id;
  int index = node->parent()->GetIndexOf(node);
  GetPrevObjectId(node->parent(), index, &prev_object_id);
  AddMetaInfo(bookmark_model, node,
              "prev_object_id", prev_object_id);

  std::string parent_order;
  std::string prev_order;
  std::string next_order;
  GetOrder(node->parent(), index, &prev_order, &next_order, &parent_order);
  AddMetaInfo(bookmark_model, node,
              "prev_order", prev_order);
  AddMetaInfo(bookmark_model, node,
              "next_order", next_order);
  AddMetaInfo(bookmark_model, node,
              "parent_order", parent_order);
  // clear the order which will be calculated when sending
  AddMetaInfo(bookmark_model, node, "order", std::string());

  std::string object_id;
  node->GetMetaInfo("object_id", &object_id);
  // newly created node
  if (object_id.empty()) {
    object_id = brave_sync::tools::GenerateObjectId();
  }
  AddMetaInfo(bookmark_model, node,
              "object_id", object_id);

  std::string parent_object_id;
  node->parent()->GetMetaInfo("object_id", &parent_object_id);
  AddMetaInfo(bookmark_model, node,
              "parent_object_id", parent_object_id);

  std::string sync_timestamp;
  node->GetMetaInfo("sync_timestamp", &sync_timestamp);
  if (sync_timestamp.empty()) {
    sync_timestamp = std::to_string(base::Time::Now().ToJsTime());
  }
  DCHECK(!sync_timestamp.empty());
  AddMetaInfo(bookmark_model, node,
              "sync_timestamp", sync_timestamp);
}

bool IsFirstLoadedFavicon(const BookmarkNode* node) {
  // Avoid sending duplicate records right after applying CREATE records,
  // BookmarkChangeProcessor::SetBookmarkFavicon, put favicon data into database
  // BookmarkNode::favicon() and BookmarkNode::icon_url() are available only
  // after first successfuly BookmarkModel::GetFavicon() which means
  // BookmarkModel::OnFaviconDataAvailable has image result available.
  // So we set metainfo to know if it is first time favicon load after create
  // node from remote record
  std::string FirstLoadedFavicon;
  if (node->GetMetaInfo("FirstLoadedFavicon", &FirstLoadedFavicon)) {
    if (!node->icon_url())
      return true;
    BookmarkNode* mutable_node = const_cast<BookmarkNode*>(node);
    mutable_node->DeleteMetaInfo("FirstLoadedFavicon");
    return true;
  }
  return false;
}

}   // namespace
}   // namespace sync_bookmarks
