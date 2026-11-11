// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_WEB_MCP_CORE_BROWSER_WEB_MCP_COMPONENT_INSTALLER_H_
#define BRAVE_COMPONENTS_WEB_MCP_CORE_BROWSER_WEB_MCP_COMPONENT_INSTALLER_H_

#include <cstdint>
#include <iterator>

#include "crypto/sha2.h"

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace web_mcp {

inline constexpr char kWebMcpComponentName[] = "Brave WebMCP Tool Scripts";
inline constexpr uint8_t kWebMcpComponentPublicKeySHA256[32] = {
    0x48, 0xd6, 0x37, 0x4b, 0xd0, 0xeb, 0x1f, 0x23, 0xa6, 0x33, 0x4a,
    0x78, 0x52, 0x95, 0xa0, 0xb5, 0xef, 0x18, 0x30, 0xed, 0xbf, 0x91,
    0xf5, 0x6e, 0xf6, 0x81, 0x8e, 0x3c, 0x34, 0x15, 0xfc, 0xfa};
static_assert(std::size(kWebMcpComponentPublicKeySHA256) ==
              crypto::kSHA256Length);

// Registers the WebMCP scripts component with the component updater. The
// delivered scripts are parsed into WebMcpRuleRegistry. Callers gate this on
// the WebMCP runtime feature being enabled.
void RegisterWebMcpComponent(component_updater::ComponentUpdateService* cus);

}  // namespace web_mcp

#endif  // BRAVE_COMPONENTS_WEB_MCP_CORE_BROWSER_WEB_MCP_COMPONENT_INSTALLER_H_
