#import "ios/web/public/webui/url_data_source_ios.h"

#define ShouldServiceRequest                                               \
  Dummy(const GURL& url) const {                                           \
    return false;                                                          \
  }                                                                        \
                                                                           \
  bool URLDataSourceIOS::ShouldAddContentSecurityPolicy() const {          \
    return true;                                                           \
  }                                                                        \
                                                                           \
  std::string URLDataSourceIOS::GetContentSecurityPolicyFrameSrc() const { \
    return "frame-src 'none';";                                            \
  }                                                                        \
                                                                           \
  bool URLDataSourceIOS::ShouldServiceRequest

#include "src/ios/web/webui/url_data_source_ios.mm"
#undef ShouldServiceRequest
