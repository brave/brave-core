#include "brave/ios/app/resources/resource_bundle.h"

#include "base/strings/sys_string_conversions.h"
#include "ui/base/resource/resource_bundle.h"

@implementation ResourceBundle
+ (NSString*)loadResource:(NSInteger)resourceId {
  auto& resource_bundle = ui::ResourceBundle::GetSharedInstance();
  
  if (resource_bundle.IsGzipped(resourceId)) {
    return base::SysUTF8ToNSString(resource_bundle.LoadDataResourceString(resourceId));
  } 
  
  return base::SysUTF8ToNSString(std::string(resource_bundle.GetRawDataResource(resourceId)));
}
@end
