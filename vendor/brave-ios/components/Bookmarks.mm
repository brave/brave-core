#include "Bookmarks.h"
#include "bookmarks/bookmarks_api.h"
#include "brave_sync/brave_sync_service.h"
#include "base/strings/utf_string_conversions.h"
#include "base/strings/sys_string_conversions.h"
#include "url/gurl.h"
#include "ios/chrome/common/string_util.h"

@interface BookmarksAPI()
{
    bookmarks::BookmarksAPI* api_;
}
@end

@implementation BookmarksAPI
- (instancetype)init:(bookmarks::BookmarksAPI *)api {
    if ((self = [super init])) {
        api_ = api;
    }
    return self;
}

- (void)dealloc {
    api_ = nullptr;
}

- (void)createWithParentId:(NSUInteger)parentId index:(NSUInteger)index title:(NSString *)title url:(NSURL *)url {
    
    base::string16 title_;
    base::UTF8ToUTF16([title UTF8String], [title length], &title_);
    
//    GURL url_ = parsedUrl(url.absoluteString.UTF8String);
//    api_->Create(parentId, index, title_, url_);
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
        sync_service_ = std::make_unique<BraveSyncService>();
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
