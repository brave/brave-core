#include "brave/ios/browser/api/bookmarks/brave_bookmarks_api.h"
#include "brave/ios/browser/api/bookmarks/brave_bookmarks_observer.h"

// #include "base/strings/utf_string_conversions.h"
#include "base/strings/sys_string_conversions.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_node.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "components/undo/bookmark_undo_service.h"
#include "components/undo/undo_manager.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "ios/chrome/browser/application_context.h"
#include "ios/chrome/browser/bookmarks/bookmark_model_factory.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state_manager.h"
#include "ios/chrome/browser/undo/bookmark_undo_service_factory.h"
#include "ios/web/public/thread/web_thread.h"
#import "net/base/mac/url_conversions.h"
#include "url/gurl.h"

// #include "ios/chrome/browser/pref_names.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface BookmarkNode()
{
  const bookmarks::BookmarkNode *node_; //UNOWNED
  bookmarks::BookmarkModel *model_; //UNOWNED
}
@end

@implementation BookmarkNode

- (instancetype)initWithNode:(const bookmarks::BookmarkNode *)node model:(bookmarks::BookmarkModel *)model {
  if ((self = [super init])) {
    self->node_ = node;
    self->model_ = model;
  }
  return self;
}

- (void)dealloc {
  self->node_ = nullptr;
  self->model_ = nullptr;
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

- (bool)isPermanentNode {
  return node_->is_permanent_node();
}

- (void)setTitle:(NSString *)title {
  model_->SetTitle(node_, base::SysNSStringToUTF16(title));
}

- (NSUInteger)nodeId {
  return node_->id();
}

//- (void)setNodeId:(NSUInteger)id {
//  node_->set_id(id);
//}

- (NSString *)getGuid {
  return base::SysUTF8ToNSString(node_->guid());
}

- (NSURL *)url {
  return net::NSURLWithGURL(node_->url());
}

- (void)setUrl:(NSURL *)url {
  model_->SetURL(node_, net::GURLWithNSURL(url));
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
    model_->SetDateAdded(node_, base::Time::FromDoubleT([date timeIntervalSince1970]));
}

- (NSDate *)dateFolderModified {
  return [NSDate dateWithTimeIntervalSince1970:node_->date_folder_modified().ToDoubleT()];
}

- (void)setDateFolderModified:(NSDate *)date {
    model_->SetDateFolderModified(node_, base::Time::FromDoubleT([date timeIntervalSince1970]));
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
  model_->SetNodeMetaInfo(node_, base::SysNSStringToUTF8(key), base::SysNSStringToUTF8(value));
}

- (void)deleteMetaInfo:(NSString *)key {
  return model_->DeleteNodeMetaInfo(node_, base::SysNSStringToUTF8(key));
}

- (NSString *)titleUrlNodeTitle {
  return base::SysUTF16ToNSString(node_->GetTitledUrlNodeTitle());
}

- (NSURL *)titleUrlNodeUrl {
  return net::NSURLWithGURL(node_->GetTitledUrlNodeUrl());
}

- (BookmarkNode *)parent {
  const bookmarks::BookmarkNode* parent_ = node_->parent();
  if (parent_) {
    return [[BookmarkNode alloc] initWithNode:parent_ model:model_];
  }
  return nil;
}

- (NSArray<BookmarkNode *> *)children {
  NSMutableArray *result = [[NSMutableArray alloc] init];
  for (const auto& child : node_->children()) {
    const bookmarks::BookmarkNode* child_node = bookmarks::GetBookmarkNodeByID(model_, static_cast<int64_t>(child->id()));
    [result addObject: [[BookmarkNode alloc] initWithNode:child_node model:model_]];
  }
  return result;
}

- (BookmarkNode *)addChildFolderWithTitle:(NSString *)title {
  if ([self isFolder]) {
    const bookmarks::BookmarkNode* node = model_->AddFolder(node_, node_->children().size(), base::SysNSStringToUTF16(title));
    return [[BookmarkNode alloc] initWithNode:node model:model_];
  }
  return nil;
}

- (BookmarkNode *)addChildBookmarkWithTitle:(NSString *)title url:(NSURL *)url {
  if ([self isFolder]) {
    const bookmarks::BookmarkNode* node = model_->AddURL(node_, node_->children().size(), base::SysNSStringToUTF16(title), net::GURLWithNSURL(url));
    return [[BookmarkNode alloc] initWithNode:node model:model_];
  }
  return nil;
}

- (void)moveToParent:(BookmarkNode *)parent {
  if ([parent isFolder]) {
    model_->Move(node_, parent->node_, parent->node_->children().size());
  }
}

- (void)moveToParent:(BookmarkNode *)parent index:(NSUInteger)index {
  if ([parent isFolder]) {
    model_->Move(node_, parent->node_, index);
  }
}

- (void)remove {
  model_->Remove(node_);
  node_ = nil;
  model_ = nil;
}

- (const bookmarks::BookmarkNode *)getNode {
  return self->node_;
}

@end

@interface BraveBookmarksAPI()
{
    bookmarks::BookmarkModel* bookmarkModel_;  // NOT OWNED
    BookmarkUndoService* bookmarkUndoService_;  // NOT OWNED
}
@end

@implementation BraveBookmarksAPI
+ (instancetype)sharedBookmarksAPI {
	static BraveBookmarksAPI *instance = nil;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		instance = [[BraveBookmarksAPI alloc] init];
	});
	return instance;
}

- (instancetype)init {
  if ((self = [super init])) {
    DCHECK_CURRENTLY_ON(web::WebThread::UI);
    ios::ChromeBrowserStateManager* browserStateManager =
        GetApplicationContext()->GetChromeBrowserStateManager();
    ChromeBrowserState* browserState =
        browserStateManager->GetLastUsedBrowserState();
    bookmarkModel_ =
        ios::BookmarkModelFactory::GetForBrowserState(browserState);
    bookmarkUndoService_ =
        ios::BookmarkUndoServiceFactory::GetForBrowserState(browserState);
  }
  return self;
}

- (void)dealloc {
  bookmarkModel_ = nil;
  bookmarkUndoService_ = nil;
}

- (BookmarkNode *)rootNode {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  const bookmarks::BookmarkNode *node = bookmarkModel_->root_node();
  if (node) {
    return [[BookmarkNode alloc] initWithNode:node model:bookmarkModel_];
  }
  return nil;
}

- (BookmarkNode *)otherNode {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  const bookmarks::BookmarkNode *node = bookmarkModel_->other_node();
  if (node) {
    return [[BookmarkNode alloc] initWithNode:node model:bookmarkModel_];
  }
  return nil;
}

- (BookmarkNode *)mobileNode {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  const bookmarks::BookmarkNode *node = bookmarkModel_->mobile_node();
  if (node) {
    return [[BookmarkNode alloc] initWithNode:node model:bookmarkModel_];
  }
  return nil;
}

- (BookmarkNode *)desktopNode {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  const bookmarks::BookmarkNode *node = bookmarkModel_->bookmark_bar_node();
  if (node) {
    return [[BookmarkNode alloc] initWithNode:node model:bookmarkModel_];
  }
  return nil;
}

- (bool)isLoaded {
  return bookmarkModel_->loaded();
}

- (id<BookmarkModelListener>)addObserver:(id<BookmarkModelObserver>)observer {
  return [[BookmarkModelListenerImpl alloc] init:observer bookmarkModel:bookmarkModel_];
}

- (void)removeObserver:(id<BookmarkModelListener>)observer {
  [observer destroy];
}

- (bool)isEditingEnabled {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  ios::ChromeBrowserStateManager* browserStateManager =
      GetApplicationContext()->GetChromeBrowserStateManager();
  ChromeBrowserState* browserState =
      browserStateManager->GetLastUsedBrowserState();

  PrefService* prefs = user_prefs::UserPrefs::Get(browserState);
  return prefs->GetBoolean(bookmarks::prefs::kEditBookmarksEnabled);
}

- (BookmarkNode *)createFolderWithTitle:(NSString *)title {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  if ([self mobileNode]) {
    return [self createFolderWithParent:[self mobileNode] title:title];
  }
  return nil;
}

- (BookmarkNode *)createFolderWithParent:(BookmarkNode *)parent title:(NSString *)title {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  DCHECK(parent);
  const bookmarks::BookmarkNode* defaultFolder = [parent getNode];

  const bookmarks::BookmarkNode* new_node = bookmarkModel_->AddFolder(defaultFolder, defaultFolder->children().size(), base::SysNSStringToUTF16(title));
  if (new_node) {
    return [[BookmarkNode alloc] initWithNode:new_node model:bookmarkModel_];
  }
  return nil;
}

- (BookmarkNode *)createBookmarkWithTitle:(NSString *)title url:(NSURL *)url {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  if ([self mobileNode]) {
    return [self createBookmarkWithParent:[self mobileNode] title:title withUrl:url];
  }
  return nil;
}

- (BookmarkNode *)createBookmarkWithParent:(BookmarkNode *)parent title:(NSString *)title withUrl:(NSURL *)url {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  DCHECK(parent);
  const bookmarks::BookmarkNode* defaultFolder = [parent getNode];

  const bookmarks::BookmarkNode* new_node = bookmarkModel_->AddURL(defaultFolder, defaultFolder->children().size(), base::SysNSStringToUTF16(title), net::GURLWithNSURL(url));
  if (new_node) {
    return [[BookmarkNode alloc] initWithNode:new_node model:bookmarkModel_];
  }
  return nil;
}

- (void)removeBookmark:(BookmarkNode *)bookmark {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  [bookmark remove];
}

- (void)removeAll {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  bookmarkModel_->RemoveAllUserBookmarks();
}

- (NSArray<BookmarkNode *> *)searchWithQuery:(NSString *)query maxCount:(NSUInteger)maxCount {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  DCHECK(bookmarkModel_->loaded());
  bookmarks::QueryFields queryFields;
  queryFields.word_phrase_query.reset(
      new base::string16(base::SysNSStringToUTF16(query)));
  std::vector<const bookmarks::BookmarkNode*> results;
  GetBookmarksMatchingProperties(
      bookmarkModel_, queryFields, maxCount, &results);

  NSMutableArray<BookmarkNode*>* nodes = [[NSMutableArray alloc] init];
  for(const bookmarks::BookmarkNode* bookmark : results) {
    BookmarkNode *node = [[BookmarkNode alloc] initWithNode:bookmark model:bookmarkModel_];
    [nodes addObject:node];
  }
  return nodes;
}

- (void)undo {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  bookmarkUndoService_->undo_manager()->Undo();
}

- (bookmarks::BookmarkModel *)getModel {
  return bookmarkModel_;
}
@end
