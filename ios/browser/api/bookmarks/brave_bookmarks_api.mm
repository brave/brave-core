#include "brave/ios/browser/api/bookmarks/brave_bookmarks_api.h"

// #include "base/strings/utf_string_conversions.h"
#include "base/strings/sys_string_conversions.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_node.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "components/undo/bookmark_undo_service.h"
#include "components/undo/undo_manager.h"
#include "ios/chrome/browser/application_context.h"
#include "ios/chrome/browser/bookmarks/bookmark_model_factory.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state_manager.h"
#include "ios/chrome/browser/undo/bookmark_undo_service_factory.h"
#import "net/base/mac/url_conversions.h"
#include "url/gurl.h"

// #include "ios/chrome/browser/pref_names.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface BookmarkNode()
{
  bookmarks::BookmarkNode *node_;
}
@end

@implementation BookmarkNode

- (instancetype)initWithNode:(bookmarks::BookmarkNode *)node {
  if ((self = [super init])) {
    self->node_ = node;
  }
  return self;
}

- (void)dealloc {
  self->node_ = nullptr;
}

+ (NSString *)kRootNodeGuid {
  return base::SysUTF8ToNSString(bookmarks::BookmarkNode::kRootNodeGuid);
}

+ (NSString *)kBookmarkBarNodeGuid {
  return base::SysUTF8ToNSString(bookmarks::BookmarkNode::kBookmarkBarNodeGuid);
}

+ (NSString *)kOtherBookmarksNodeGuid {
  return base::SysUTF8ToNSString(bookmarks::BookmarkNode::kOtherBookmarksNodeGuid);
}

+ (NSString *)kMobileBookmarksNodeGuid {
  return base::SysUTF8ToNSString(bookmarks::BookmarkNode::kMobileBookmarksNodeGuid);
}

+ (NSString *)kManagedNodeGuid {
  return base::SysUTF8ToNSString(bookmarks::BookmarkNode::kManagedNodeGuid);
}

+ (NSString *)RootNodeGuid {
  return base::SysUTF8ToNSString(bookmarks::BookmarkNode::RootNodeGuid());
}

- (bool)isPermanentNode {
  return node_->is_permanent_node();
}

- (void)setTitle:(NSString *)title {
  return node_->SetTitle(base::SysNSStringToUTF16(title));
}

- (NSUInteger)nodeId {
  return node_->id();
}

- (void)setNodeId:(NSUInteger)id {
  node_->set_id(id);
}

- (NSString *)getGuid {
  return base::SysUTF8ToNSString(node_->guid());
}

- (NSURL *)url {
  return net::NSURLWithGURL(node_->url());
}

- (void)setUrl:(NSURL *)url {
  node_->set_url(net::GURLWithNSURL(url));
}

- (NSURL *)iconUrl {
  const GURL* url = node_->icon_url();
  return url ? net::NSURLWithGURL(*url) : nullptr;
}

- (BookmarksNodeType)type {
  switch (node_->type()) {
    case bookmarks::BookmarkNode::URL: return BookmarksNodeType::URL;
    case bookmarks::BookmarkNode::FOLDER: return BookmarksNodeType::FOLDER;
    case bookmarks::BookmarkNode::BOOKMARK_BAR: return BookmarksNodeType::BOOKMARK_BAR;
    case bookmarks::BookmarkNode::OTHER_NODE: return BookmarksNodeType::OTHER_NODE;
    case bookmarks::BookmarkNode::MOBILE: return BookmarksNodeType::MOBILE;
  }
  return BookmarksNodeType::MOBILE;
}

- (NSDate *)dateAdded {
  return [NSDate dateWithTimeIntervalSince1970:node_->date_added().ToDoubleT()];
}

- (void)setDateAdded:(NSDate *)date {
    node_->set_date_added(base::Time::FromDoubleT([date timeIntervalSince1970]));
}

- (NSDate *)dateFolderModified {
  return [NSDate dateWithTimeIntervalSince1970:node_->date_folder_modified().ToDoubleT()];
}

- (void)setDateFolderModified:(NSDate *)date {
    node_->set_date_folder_modified(base::Time::FromDoubleT([date timeIntervalSince1970]));
}

- (bool)isFolder {
  return node_->is_folder();
}

- (bool)isUrl {
  return node_->is_url();
}

- (bool)isFavIconLoaded {
  return node_->is_favicon_loaded();
}

- (bool)isFavIconLoading {
  return node_->is_favicon_loading();
}

- (bool)isVisible {
  return node_->IsVisible();
}

- (bool)getMetaInfo:(NSString *)key value:(NSString **)value {
  std::string value_;
  bool result = node_->GetMetaInfo(base::SysNSStringToUTF8(key), &value_);
  if (value) {
    *value = base::SysUTF8ToNSString(value_);
  }
  return result;
}

- (void)setMetaInfo:(NSString *)key value:(NSString *)value {
  node_->SetMetaInfo(base::SysNSStringToUTF8(key), base::SysNSStringToUTF8(value));
}

- (bool)deleteMetaInfo:(NSString *)key {
  return node_->DeleteMetaInfo(base::SysNSStringToUTF8(key));
}

- (NSString *)titleUrlNodeTitle {
  return base::SysUTF16ToNSString(node_->GetTitledUrlNodeTitle());
}

- (NSURL *)titleUrlNodeUrl {
  return net::NSURLWithGURL(node_->GetTitledUrlNodeUrl());
}

- (bookmarks::BookmarkNode *)getInternalNode {
  return self->node_;
}

@end

@interface BraveBookmarksAPI()
{
    bookmarks::BookmarkModel* _bookmarkModel;  // NOT OWNED
    BookmarkUndoService* _bookmarkUndoService;  // NOT OWNED
}
@end

@implementation BraveBookmarksAPI
- (instancetype)init {
  if ((self = [super init])) {
    ios::ChromeBrowserStateManager* browserStateManager =
        GetApplicationContext()->GetChromeBrowserStateManager();
    ChromeBrowserState* browserState =
        browserStateManager->GetLastUsedBrowserState();
    _bookmarkModel =
        ios::BookmarkModelFactory::GetForBrowserState(browserState);
    _bookmarkUndoService =
        ios::BookmarkUndoServiceFactory::GetForBrowserState(browserState);
  }
  return self;
}

- (void)dealloc {
    _bookmarkModel = nil;
}

- (void)createWithParentId:(NSUInteger)parentId index:(NSUInteger)index title:(NSString *)title url:(NSURL *)url {
    // const bookmarks::BookmarkNode* defaultFolder = api_->GetBookmarksMobileFolder();
    // _bookmarkModel->AddURL(defaultFolder, defaultFolder->children().size(),
    //                       base::SysNSStringToUTF16(title), net::GURLWithNSURL(url));
}

- (void)moveWithId:(NSUInteger)bookmarkId parentId:(NSUInteger)parentId index:(NSUInteger)index {
    // _bookmarkModel->Move(bookmark, static_cast<size_t>(parentId), index);
}

- (void)updateWithId:(NSUInteger)bookmarkId title:(NSString *)title url:(NSURL *)url {
    // _bookmarkModel->Update(bookmark, base::SysNSStringToUTF16(title), net::GURLWithNSURL(url));
}

- (void)removeWithId:(NSUInteger)bookmarkId {
    // _bookmarkModel->Remove(bookmark);
}

- (void)removeAll {
    _bookmarkModel->RemoveAllUserBookmarks();
}

- (NSArray<BookmarkNode *> *)searchWithQuery:(NSString *)query maxCount:(NSUInteger)maxCount {
  DCHECK(_bookmarkModel->loaded());
  bookmarks::QueryFields queryFields;
  queryFields.word_phrase_query.reset(
      new base::string16(base::SysNSStringToUTF16(query)));
  std::vector<const bookmarks::BookmarkNode*> results;
  GetBookmarksMatchingProperties(
      _bookmarkModel, queryFields, maxCount, &results);

  NSMutableArray<BookmarkNode*>* nodes = [[NSMutableArray alloc] init];
  // convert bookmarks::BookmarkNode -> BookmarkNode
  return nodes;
}

- (void)undo {
    _bookmarkUndoService->undo_manager()->Undo();
}

- (void)addBookmark:(NSString *)title url:(NSURL *)url {
    // const bookmarks::BookmarkNode* defaultFolder = api_->GetBookmarksMobileFolder();
    // _bookmarkModel->AddURL(defaultFolder, defaultFolder->children().size(),
    //                       base::SysNSStringToUTF16(title), net::GURLWithNSURL(url));
}

@end
