#include "brave/vendor/brave-ios/components/LocalDeviceInfo.h"

#include "base/strings/sys_string_conversions.h"
#include "brave/ios/browser/browser_state/browser_state_manager.h"
#include "brave/vendor/brave-ios/components/brave_sync/brave_sync_service.h"
#include "components/sync_device_info/device_info.h"
#include "components/sync_device_info/device_info_sync_service.h"
#include "components/sync_device_info/local_device_info_provider_impl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface LocalDeviceInfo()
{
    const syncer::DeviceInfo *local_device_;
}
@end

@implementation LocalDeviceInfo
- (instancetype)init:(const syncer::DeviceInfo *)info {
    if ((self = [super init])) {
        self->local_device_ = info;
    }
    return self;
}

- (NSString *)getGuid {
    return base::SysUTF8ToNSString(local_device_->guid().c_str());
}
@end

@interface DeviceInfoService()
{
  std::unique_ptr<BraveSyncService> sync_service_;
}
@end

@implementation DeviceInfoService
- (instancetype)init {
  if ((self = [super init])) {
    ChromeBrowserState* browser_state_ = BrowserStateManager::GetInstance().GetBrowserState();
    CHECK(browser_state_);
    sync_service_ = std::make_unique<BraveSyncService>(browser_state_);
    CHECK(sync_service_.get());
  }
  return self;
}

- (void)dealloc {
    sync_service_.reset();
}

/*- (void) initLocalDevice:(NSString *)guid name:(NSString *)name manufacturer:(NSString *)manufacturer model:(NSString *)model  {
	auto* local_device_info_provider = sync_service_->device_info_service()->GetLocalDeviceInfoProvider();
	local_device_info_provider->Initialize(base::SysNSStringToUTF8(guid),
		                                   base::SysNSStringToUTF8(name),
		                                   base::SysNSStringToUTF8(manufacturer),
		                                   base::SysNSStringToUTF8(model)
		                                   );
}*/

- (LocalDeviceInfo *)getLocalDeviceInfo {
	auto* local_device_info_provider = sync_service_->device_info_service()->GetLocalDeviceInfoProvider();
    return [[LocalDeviceInfo alloc] init:local_device_info_provider->GetLocalDeviceInfo()];
}
@end
