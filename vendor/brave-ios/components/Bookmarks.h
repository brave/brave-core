
#import <Foundation/Foundation.h>

//NS_SWIFT_NAME(BookmarksAPI)
@interface BookmarksAPI: NSObject
@end

//NS_SWIFT_NAME(BookmarksService)
@interface BookmarksService: NSObject
- (BookmarksAPI *)create;
@end
