#ifndef BRAVE_BNES_BNS_CONSTANTS_H_
#define BRAVE_BNES_BNS_CONSTANTS_H_

#include <string_view>

namespace bns {

inline constexpr std::string_view kBnesScheme = "bnes";
inline constexpr std::string_view kBnesHostSuffix = ".bnes";
inline constexpr std::string_view kDefaultIpfsGatewayHost =
    "ipfs.bearnetwork.net";
inline constexpr std::string_view kIpfsPathPrefix = "/ipfs/";
inline constexpr std::string_view kHttpsScheme = "https";

}  // namespace bns

#endif  // BRAVE_BNES_BNS_CONSTANTS_H_
