#ifndef BRAVE_BNES_BNS_SECURITY_H_
#define BRAVE_BNES_BNS_SECURITY_H_

#include <string_view>

#include "url/gurl.h"

namespace bns {

bool IsValidBnesHost(std::string_view host);
bool IsAllowedNavigationUrl(const GURL& url);
bool IsValidGatewayOrigin(const GURL& origin);

}  // namespace bns

#endif  // BRAVE_BNES_BNS_SECURITY_H_
