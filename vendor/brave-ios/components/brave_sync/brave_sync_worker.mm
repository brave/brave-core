
#include "brave/vendor/brave-ios/components/brave_sync/brave_sync_worker.h"
#include "brave/ios/browser/browser_state/browser_state_manager.h"
#include "brave/vendor/brave-ios/components/brave_sync/brave_sync_service.h"

#import <CoreImage/CoreImage.h>
#include <string>
#include <vector>

#include "base/strings/sys_string_conversions.h"
#include "base/json/json_writer.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"

#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "brave/components/brave_sync/crypto/crypto.h"

//#include "chrome/browser/sync/device_info_sync_service_factory.h"
//#include "chrome/browser/sync/profile_sync_service_factory.h"

#include "components/sync/driver/sync_service.h"
#include "components/sync/driver/sync_user_settings.h"
#include "components/sync_device_info/device_info.h"
#include "components/sync_device_info/device_info_sync_service.h"
#include "components/sync_device_info/device_info_tracker.h"
#include "components/sync_device_info/local_device_info_provider.h"
#include "components/unified_consent/unified_consent_metrics.h"

#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/sync/profile_sync_service_factory.h"
#include "ios/chrome/browser/sync/device_info_sync_service_factory.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

//namespace {
//static const size_t SEED_BYTES_COUNT = 32u;
//}  // namespace


#include "base/scoped_observer.h"
#include "components/sync_device_info/device_info_tracker.h"

namespace chrome {
namespace ios {
class BraveSyncDevicesIOS : public syncer::DeviceInfoTracker::Observer {
 public:
  BraveSyncDevicesIOS();
  virtual ~BraveSyncDevicesIOS();

  void Destroy();

 private:
  // syncer::DeviceInfoTracker::Observer
  void OnDeviceInfoChange() override;
    
  ScopedObserver<syncer::DeviceInfoTracker, syncer::DeviceInfoTracker::Observer>
      device_info_tracker_observer_{this};

  ChromeBrowserState* browser_state_ = nullptr;
};

}
}

namespace chrome {
namespace ios {

BraveSyncDevicesIOS::BraveSyncDevicesIOS() {
  browser_state_ = BrowserStateManager::GetInstance().GetBrowserState();
  CHECK(browser_state_);

  syncer::DeviceInfoTracker* tracker =
    DeviceInfoSyncServiceFactory::GetForBrowserState(browser_state_)
       ->GetDeviceInfoTracker();
  DCHECK(tracker);
  if (tracker) {
    device_info_tracker_observer_.Add(tracker);
  }
}

BraveSyncDevicesIOS::~BraveSyncDevicesIOS() {
  // Observer will be removed by ScopedObserver
}

void BraveSyncDevicesIOS::Destroy() {
  delete this;
}

void BraveSyncDevicesIOS::OnDeviceInfoChange() {
  fprintf(stderr, "DEVICE INFO CHANGED\n");
}

}  // namespace ios
}  // namespace chrome


@interface BraveSyncWorker()
{
    ChromeBrowserState* browser_state_;
    chrome::ios::BraveSyncDevicesIOS* device_;
}
@end

@implementation BraveSyncWorker
- (instancetype)init {
    if ((self = [super init])) {
        //DCHECK(false);
        browser_state_ = BrowserStateManager::GetInstance().GetBrowserState();
        CHECK(browser_state_);
        
        device_ = new chrome::ios::BraveSyncDevicesIOS();
    }
    return self;
}

- (void)dealloc {
    device_->Destroy();
    browser_state_ = nullptr;
}

- (NSString *)getSyncCodeWords {
    brave_sync::Prefs brave_sync_prefs(browser_state_->GetPrefs());
    std::string sync_code = brave_sync_prefs.GetSeed();

    if (sync_code.empty()) {
      std::vector<uint8_t> seed = brave_sync::crypto::GetSeed();
      sync_code = brave_sync::crypto::PassphraseFromBytes32(seed);
      brave_sync_prefs.SetSeed(sync_code);
        
      VLOG(3) << "[BraveSync] " << __PRETTY_FUNCTION__ << " generated new sync code";
    }
    return base::SysUTF8ToNSString(sync_code.c_str());
}

- (bool)setSyncCodeWords:(NSString *)passphrase {
    std::vector<uint8_t> seed;
    std::string code_words = base::SysNSStringToUTF8(passphrase);
    
    if (!brave_sync::crypto::PassphraseToBytes32(code_words, &seed)) {
      LOG(ERROR) << "[BraveSync] Invalid sync code: " << code_words;
      return false;
    }
    
    brave_sync::Prefs brave_sync_prefs{browser_state_->GetPrefs()};
    brave_sync_prefs.SetSeed(code_words);
    return true;
}

- (UIImage *)getQRCodeImage:(NSString *)passphrase withSize:(CGSize)size {
    std::vector<uint8_t> seed;
    std::string code_words = base::SysNSStringToUTF8(passphrase);
    if (!brave_sync::crypto::PassphraseToBytes32(code_words, &seed)) {
        LOG(ERROR) << "[BraveSync] Invalid sync code when generating QRCode";
        return nil;
    }
    
    // QR code version 3 can only carry 84 bytes so we hex encode 32 bytes
    // seed then we will have 64 bytes input data
    const std::string sync_code_hex = base::HexEncode(seed.data(), seed.size());
    
    NSData *sync_code_data = [base::SysUTF8ToNSString(sync_code_hex.c_str()) dataUsingEncoding: NSUTF8StringEncoding]; //NSISOLatin1StringEncoding
    
    if (!sync_code_data) {
      LOG(ERROR) << "[BraveSync] Failed to convert sync_code_hex to NSData for QRCode generation";
      return nil;
    }

    CIFilter *filter = [CIFilter filterWithName:@"CIQRCodeGenerator"];
    [filter setValue:sync_code_data forKey:@"inputMessage"];
    [filter setValue:@"H" forKey:@"inputCorrectionLevel"];
    
    CIImage *ciImage = [filter outputImage];
    if (ciImage) {
      CGFloat scaleX = size.width / ciImage.extent.size.width;
      CGFloat scaleY = size.height / ciImage.extent.size.height;
      CGAffineTransform transform = CGAffineTransformMakeScale(scaleX, scaleY);
      ciImage = [ciImage imageByApplyingTransform:transform];
    }

    if (ciImage) {
        return [UIImage imageWithCIImage:ciImage scale:[[UIScreen mainScreen] scale] orientation:UIImageOrientationUp];
    }
    
    LOG(ERROR) << "[BraveSync] Failed to generate QRCode";
    return nil;
}

- (syncer::SyncService *)getSyncService {
    return static_cast<syncer::SyncService*>(
                                             ProfileSyncServiceFactory::GetForBrowserState(browser_state_));
}

- (syncer::DeviceInfoTracker *)getDeviceInfoTracker {
  auto* device_info_sync_service =
      DeviceInfoSyncServiceFactory::GetForBrowserState(browser_state_);
  return device_info_sync_service->GetDeviceInfoTracker();
}

- (syncer::LocalDeviceInfoProvider *)getLocalDeviceInfoProvider {
  auto* device_info_sync_service =
      DeviceInfoSyncServiceFactory::GetForBrowserState(browser_state_);
  return device_info_sync_service->GetLocalDeviceInfoProvider();
}

- (base::Value) getDeviceList {
  auto* device_info_service =
      DeviceInfoSyncServiceFactory::GetForBrowserState(browser_state_);
  syncer::DeviceInfoTracker* tracker =
      device_info_service->GetDeviceInfoTracker();
  DCHECK(tracker);
    
  const syncer::DeviceInfo* local_device_info = device_info_service
     ->GetLocalDeviceInfoProvider()->GetLocalDeviceInfo();

  base::Value device_list(base::Value::Type::LIST);

//    const std::vector<std::unique_ptr<syncer::DeviceInfo>> all_devices =
//       tracker->GetAllDeviceInfo();

  fprintf(stderr, "DEVICE INFO: %p\n", local_device_info);
  for (const auto& device : tracker->GetAllDeviceInfo()) {
    auto device_value = base::Value::FromUniquePtrValue(device->ToValue());
    bool is_current_device = local_device_info
        ? local_device_info->guid() == device->guid()
        : false;
    device_value.SetBoolKey("isCurrentDevice", is_current_device);
    device_list.Append(std::move(device_value));
  }

  return device_list;
}

- (NSString *)getDeviceListJSON {
    std::string json_string;
    base::Value device_list = [self getDeviceList];
    
    if (!base::JSONWriter::Write(device_list, &json_string)) {
      VLOG(1) << "[BraveSync] Writing as Device List to JSON failed.";
    }
    
    if (!json_string.empty()) {
      return base::SysUTF8ToNSString(json_string.c_str());
    }
    return nil;
}

- (void)reset {
    auto* sync_service = [self getSyncService];
    
    if (!sync_service || sync_service->GetTransportState() !=
        syncer::SyncService::TransportState::ACTIVE) {
      [self onSelfDeleted];
      return;
    }

    syncer::DeviceInfoTracker* tracker = [self getDeviceInfoTracker];
    DCHECK(tracker);
    
    const syncer::DeviceInfo* local_device_info =
        [self getLocalDeviceInfoProvider]->GetLocalDeviceInfo();

    (void)local_device_info;
//    tracker->DeleteDeviceInfo(local_device_info->guid(),
//                              base::BindOnce(&onSelfDeleted,
//                                             weak_ptr_factory_.GetWeakPtr(),
//                                             std::move(callback_id_arg)));
}

- (void)onSelfDeleted {
//    auto* sync_service = [self getSyncService];
//    if (sync_service) {
//      sync_service->StopAndClear();
//    }
//    brave_sync::Prefs brave_sync_prefs(profile_->GetPrefs());
//    brave_sync_prefs.Clear();
//    // Sync prefs will be clear in ProfileSyncService::StopImpl
}

- (void)start {
    syncer::SyncService* service =
        ProfileSyncServiceFactory::GetForBrowserState(browser_state_);

//    if (service && !sync_service_observer_.IsObserving(service)) {
//      sync_service_observer_.Add(service);
//    }

    if (service) {
      service->GetUserSettings()->SetSyncRequested(true);
    }
}

- (bool)isFirstSetupComplete {
    syncer::SyncService* sync_service = [self getSyncService];
    return sync_service &&
           sync_service->GetUserSettings()->IsFirstSetupComplete();
}

- (void)finalizeSetup {
    syncer::SyncService* service = [self getSyncService];
    if (!service)
      return;

    service->GetUserSettings()->SetSyncRequested(true);
    if (service->GetUserSettings()->IsFirstSetupComplete())
      return;

    unified_consent::metrics::RecordSyncSetupDataTypesHistrogam(
        service->GetUserSettings(), browser_state_->GetPrefs());

    service->GetUserSettings()->SetFirstSetupComplete(
        syncer::SyncFirstSetupCompleteSource::ADVANCED_FLOW_CONFIRM);
}
@end
