#include "brave/bnes/bns_security.h"

#include "base/strings/string_util.h"
#include "url/gurl.h"

namespace bns {

inline constexpr std::string_view kBnesScheme = "bnes";
inline constexpr std::string_view kBnesHostSuffix = ".bnes";
inline constexpr std::string_view kDefaultIpfsGatewayHost =
    "ipfs.bearnetwork.net";
inline constexpr std::string_view kIpfsPathPrefix = "/ipfs/";
inline constexpr std::string_view kHttpsScheme = "https";

bool IsValidBnesHost(std::string_view host) {
  if (host.empty()) {
    return false;
  }
  if (host == ".bnes") {
    return false;
  }
  if (host.find("..") != std::string_view::npos) {
    return false;
  }
  if (host.front() == '.' || host.back() == '.') {
    return false;
  }
  if (!base::EndsWith(host, kBnesHostSuffix)) {
    return false;
  }
  return true;
}

bool IsAllowedNavigationUrl(const GURL& url) {
  if (!url.is_valid()) {
    return false;
  }
  if (!url.SchemeIs(kBnesScheme)) {
    return false;
  }
  return IsValidBnesHost(url.host());
}

bool IsValidGatewayOrigin(const GURL& origin) {
  if (!origin.is_valid()) {
    return false;
  }
  if (!origin.SchemeIs(kHttpsScheme)) {
    return false;
  }
  if (origin.host() != kDefaultIpfsGatewayHost) {
    return false;
  }
  if (!origin.port().empty() && origin.port() != "443") {
    return false;
  }
  return true;
}

}  // namespace bns
