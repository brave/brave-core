#import <Foundation/Foundation.h>


@interface BraveCoreShared: NSObject
+ (instancetype)shared NS_SWIFT_NAME(shared());
- (instancetype)init NS_UNAVAILABLE;

- (NSString *)getUserAgent;
- (void)setUserAgentCallback:(NSString*(^)())userAgentCallback;
@end
