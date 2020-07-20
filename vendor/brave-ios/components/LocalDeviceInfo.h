#import <Foundation/Foundation.h>

@interface LocalDeviceInfo: NSObject
- (NSString *)getGuid;
@end

//NS_SWIFT_NAME(DeviceInfoService)
@interface DeviceInfoService: NSObject
- (LocalDeviceInfo *)getLocalDeviceInfo;
//- (void) initLocalDevice:(NSString *)guid name:(NSString *)name manufacturer:(NSString *)manufacturer model:(NSString *)model;
@end