#import "RewardsLogger.h"
#import "RewardsLogStream.h"

@implementation BATBraveRewardsLogger
+ (void)configureWithLogCallback:(void(^)(BATLogLevel logLevel, int line, NSString *file, NSString *data))onWrite withFlush:(nullable void (^)())onFlush
{
  /// Bridge the Unbuffered Callback to iOS
  UnbufferedLogger::setLoggerCallbacks([onWrite](UnbufferedLoggerData logData) {
    onWrite(static_cast<BATLogLevel>(logData.log_level),
            logData.line,
            [NSString stringWithUTF8String: logData.file.c_str()],
            [NSString stringWithUTF8String: logData.data.c_str()]);
  }, [onFlush]() {
    if (onFlush) {
      onFlush();
    }
  });
}
@end
