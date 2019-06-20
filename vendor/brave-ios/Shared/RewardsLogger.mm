#import "RewardsLogger.h"
#import "RewardsLogStream.h"

@implementation BATBraveRewardsLogger
+ (void)configureWithLogCallback:(void(^)(BATLogLevel logLevel, int line, NSString *file, NSString *data))onWrite withFlush:(void(^)())onFlush {
    
  /// Bridge the Unbuffered Callback to iOS
  UnbufferedLogger::setLoggerCallbacks([onWrite](UnbufferedLoggerData logData) {
    onWrite(logData.log_level,
            logData.line,
            [NSString stringWithUTF8String: logData.file.c_str()],
            [NSString stringWithUTF8String: logData.data.c_str()]);
  }, [onFlush]() {
    onFlush();
  });
}
@end
