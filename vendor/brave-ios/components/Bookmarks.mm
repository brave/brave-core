#include "brave/vendor/brave-ios/components/Bookmarks.h"

#include "base/files/file_path.h"
#include "base/strings/utf_string_conversions.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/vendor/brave-ios/components/bookmarks/bookmarks_api.h"
#include "brave/vendor/brave-ios/components/brave_sync/brave_sync_service.h"
#include "brave/ios/browser/browser_state/browser_state_manager.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/web/public/init/web_main.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_node.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "url/gurl.h"
#import "net/base/mac/url_conversions.h"

#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "ios/chrome/browser/bookmarks/bookmark_model_factory.h"
#include "ios/chrome/browser/pref_names.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface BookmarksNode()
{
    bookmarks::BookmarkNode *node_;
}
@end

@implementation BookmarksNode

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
    return base::SysUTF8ToNSString(bookmarks::BookmarkNode::RootNodeGuid().c_str());
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
    return base::SysUTF8ToNSString(node_->guid().c_str());
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
    if (value)
    {
        *value = base::SysUTF8ToNSString(value_.c_str());
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
    return base::SysUTF16ToNSString(node_->GetTitledUrlNodeTitle().c_str());
}

- (NSURL *)titleUrlNodeUrl {
    return net::NSURLWithGURL(node_->GetTitledUrlNodeUrl());
}

- (bookmarks::BookmarkNode *)getInternalNode {
    return self->node_;
}
@end

@interface BookmarksAPI()
{
    bookmarks::BookmarksAPI* api_;
}
@end

@implementation BookmarksAPI
- (instancetype)init:(bookmarks::BookmarksAPI *)api {
    if ((self = [super init])) {
        self->api_ = api;
    }
    return self;
}

- (void)dealloc {
    self->api_ = nullptr;
}

- (void)createWithParentId:(NSUInteger)parentId index:(NSUInteger)index title:(NSString *)title url:(NSURL *)url {
    ChromeBrowserState* browser_state_ = BrowserStateManager::GetInstance().GetBrowserState();

    const bookmarks::BookmarkNode* defaultFolder = api_->GetBookmarksMobileFolder();
    bookmarks::BookmarkModel* bookmarkModel =
        ios::BookmarkModelFactory::GetForBrowserState(browser_state_);
    bookmarkModel->AddURL(defaultFolder, defaultFolder->children().size(),
                          base::SysNSStringToUTF16(title), net::GURLWithNSURL(url));
}

- (void)moveWithId:(NSUInteger)bookmarkId parentId:(NSUInteger)parentId index:(NSUInteger)index {
    api_->Move(bookmarkId, parentId, index);
}

- (void)updateWithId:(NSUInteger)bookmarkId title:(NSString *)title url:(NSURL *)url {
    api_->Update(bookmarkId, base::SysNSStringToUTF16(title), net::GURLWithNSURL(url));
}

- (void)removeWithId:(NSUInteger)bookmarkId {
    api_->Remove(bookmarkId);
}

- (void)removeAll {
    api_->RemoveAll();
}

- (void)searchWithQuery:(NSString *)query maxCount:(NSUInteger)maxCount nodes:(NSArray<BookmarksNode *> *)nodes {
    
    std::vector<const bookmarks::BookmarkNode *> nodes_;
    for (NSUInteger i = 0; i < [nodes count]; ++i) {
        nodes_.push_back([nodes[i] getInternalNode]);
    }
    
    api_->Search(base::SysNSStringToUTF16(query), maxCount, &nodes_);
}

- (void)undo {
    api_->Undo();
}

- (void)addBookmark:(NSString *)title url:(NSURL *)url {
    ChromeBrowserState* browser_state_ = BrowserStateManager::GetInstance().GetBrowserState();

    const bookmarks::BookmarkNode* defaultFolder = api_->GetBookmarksMobileFolder();
    bookmarks::BookmarkModel* bookmarkModel =
        ios::BookmarkModelFactory::GetForBrowserState(browser_state_);
    bookmarkModel->AddURL(defaultFolder, defaultFolder->children().size(),
                          base::SysNSStringToUTF16(title), net::GURLWithNSURL(url));
}
@end


@interface BookmarksService()
{
  std::unique_ptr<BraveSyncService> sync_service_;
}
@end

@implementation BookmarksService
- (instancetype)init {
  if ((self = [super init])) {
    ChromeBrowserState* browser_state_ = BrowserStateManager::GetInstance().GetBrowserState();
    CHECK(browser_state_);
    sync_service_ = std::make_unique<BraveSyncService>(browser_state_);
    CHECK(sync_service_.get());
  }
  return self;
}

- (void)dealloc {
    sync_service_.reset();
}

- (BookmarksAPI *)create {
    return [[BookmarksAPI alloc] init:sync_service_->bookmarks_api()];
}
@end
