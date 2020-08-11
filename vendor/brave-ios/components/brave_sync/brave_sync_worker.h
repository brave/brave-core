#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@interface BraveSyncWorker: NSObject
- (instancetype)init;

- (NSString *)getSyncCodeWords;
- (bool)setSyncCodeWords:(NSString *)passphrase;
- (UIImage *)getQRCodeImage:(NSString *)passphrase withSize:(CGSize)size;
- (NSString *)getDeviceListJSON;
- (void)reset;
- (void)start;
- (bool)isFirstSetupComplete;
- (void)finalizeSetup;
@end
