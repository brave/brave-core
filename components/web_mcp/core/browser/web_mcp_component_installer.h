// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_WEB_MCP_CORE_BROWSER_WEB_MCP_COMPONENT_INSTALLER_H_
#define BRAVE_COMPONENTS_WEB_MCP_CORE_BROWSER_WEB_MCP_COMPONENT_INSTALLER_H_

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace web_mcp {

inline constexpr char kWebMcpComponentName[] = "Brave WebMCP Tool Scripts";
inline constexpr char kWebMcpComponentId[] = "kddaehjleefhcbmnkdmjnhiphbomedpf";
inline constexpr char kWebMcpComponentBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAvX8cJN/"
    "GXZCwijNam8Pyqb+gFi2GJE+"
    "cN6cTfvDmToSEivqQ3SQkLZwyORIWKxxrWnopybF2MzUEOLoW1gPjGGGrZVExlcIpLHX0SitFq"
    "d"
    "fQDMOSdVD5OOJEvV/YOKyejmWjXcIAhj5/"
    "J8d+lAvHINCbQQXMvWvnzeo6BBcVrYh6sbg6yDAQ8"
    "5aoANTDBIffjui8c+wSnfQdZ9B2gP4JRX48iDWYpE2z2Wk/"
    "q0hecXK4UQbn7BbZmWwChfJWL64E"
    "+BiGdBV1yhdzJHmXo+05wWPzFB2SjxUjXfnht+fgypeTwNGvBTFBM1qHu3VZ/"
    "+pZggjUIdMCXUK"
    "ItqYI5fC8GwIDAQAB";

// Registers the WebMCP scripts component with the component updater. The
// delivered scripts are parsed into WebMcpRuleRegistry. Callers gate this on
// the WebMCP runtime feature being enabled.
void RegisterWebMcpComponent(component_updater::ComponentUpdateService* cus);

}  // namespace web_mcp

#endif  // BRAVE_COMPONENTS_WEB_MCP_CORE_BROWSER_WEB_MCP_COMPONENT_INSTALLER_H_
