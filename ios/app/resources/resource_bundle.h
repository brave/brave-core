#ifndef BRAVE_IOS_APP_RESOURCES_RESOURCE__BUNDLE_H_
#define BRAVE_IOS_APP_RESOURCES_RESOURCE__BUNDLE_H_

#import <Foundation/Foundation.h>

OBJC_EXPORT
@interface ResourceBundle: NSObject
+ (NSString*)loadResource:(NSInteger)resourceId;
@end

#endif  // BRAVE_IOS_APP_RESOURCES_RESOURCE__BUNDLE_H_
