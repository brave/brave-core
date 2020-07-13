
#import <Foundation/Foundation.h>

typedef NS_ENUM(NSUInteger, BookmarksNodeType) {
  URL,
  FOLDER,
  BOOKMARK_BAR,
  OTHER_NODE,
  MOBILE
};

typedef NS_ENUM(NSUInteger, BookmarksNodeFaviconState) {
  INVALID_FAVICON,
  LOADING_FAVICON,
  LOADED_FAVICON,
};

@interface BookmarksNode: NSObject
+ (NSString *)kRootNodeGuid;
+ (NSString *)kBookmarkBarNodeGuid;
+ (NSString *)kOtherBookmarksNodeGuid;
+ (NSString *)kMobileBookmarksNodeGuid;
+ (NSString *)kManagedNodeGuid;

+ (NSString *)RootNodeGuid;
- (bool)isPermanentNode;
- (void)setTitle:(NSString *)title;
- (NSUInteger)nodeId;
- (void)setNodeId:(NSUInteger)id;
- (NSString *)getGuid;
- (NSURL *)url;
- (void)setUrl:(NSURL *)url;

- (NSURL *)iconUrl;
- (BookmarksNodeType)type;
- (NSDate *)dateAdded;
- (void)setDateAdded:(NSDate *)date;

- (NSDate *)dateFolderModified;
- (void)setDateFolderModified:(NSDate *)date;
- (bool)isFolder;
- (bool)isUrl;
- (bool)isFavIconLoaded;
- (bool)isFavIconLoading;
- (bool)isVisible;

- (bool)getMetaInfo:(NSString *)key value:(NSString **)value;
- (void)setMetaInfo:(NSString *)key value:(NSString *)value;
- (bool)deleteMetaInfo:(NSString *)key;

- (NSString *)titleUrlNodeTitle;
- (NSURL *)titleUrlNodeUrl;
@end

//NS_SWIFT_NAME(BookmarksAPI)
@interface BookmarksAPI: NSObject
- (void)createWithParentId:(NSUInteger)parentId index:(NSUInteger)index title:(NSString *)title url:(NSURL *)url;

- (void)moveWithId:(NSUInteger)bookmarkId parentId:(NSUInteger)parentId index:(NSUInteger)index;

- (void)updateWithId:(NSUInteger)bookmarkId title:(NSString *)title url:(NSURL *)url;

- (void)removeWithId:(NSUInteger)bookmarkId;

- (void)removeAll;

- (void)searchWithQuery:(NSString *)query maxCount:(NSUInteger)maxCount nodes:(NSArray<BookmarksNode *> *)nodes;

- (void)undo;

- (void)addBookmark:(NSString *)title url:(NSURL *)url;
@end

//NS_SWIFT_NAME(BookmarksService)
@interface BookmarksService: NSObject
- (BookmarksAPI *)create;
@end
