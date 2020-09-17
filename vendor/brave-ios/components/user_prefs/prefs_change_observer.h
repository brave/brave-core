
#import <Foundation/Foundation.h>

@interface BravePrefsChangeObserver: NSObject
- (instancetype)initWithObserver:(void(^)())observer;
@end
