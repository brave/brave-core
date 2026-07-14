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
inline constexpr char kWebMcpComponentId[] = "eingdhelnaolbpcdkgddekhifcjfkalf";
inline constexpr char kWebMcpComponentBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA1cs/q1SFnqEgbPFni1hTXJNDqzsf"
    "JJIzNoUNCD/vPwpgUH0nXvLAgagAyBcxFi3Y7rU3vidnXiFL7iPaikZN40nEfsfE/3FNaIaH"
    "PQa3xtmL76CqsAT21iYuEu+I3MZIqTNewC7XUqBh5rw1V0joVkQvHMpeKqUgIUhnxeqjQpoy"
    "L3GLR9d3P+f5g8V206dJUvXqUKc2Cc5tVYpSHIiIhh081fchkYwJ9Yszg3EIhCgnbaoONAHs"
    "jzfGsJTQ6IlgD2uyOdtQi4/wdlsWNvvlpqLNIVLTIyAy8QlCOTujvyey95CJPF2e/6LuIzky"
    "KUaje/tFmyDD9xTTcpRSKMVY8QIDAQAB";

// Registers the WebMCP scripts component with the component updater. The
// delivered scripts are parsed into WebMcpRuleRegistry. Callers gate this on
// the WebMCP runtime feature being enabled.
void RegisterWebMcpComponent(component_updater::ComponentUpdateService* cus);

}  // namespace web_mcp

#endif  // BRAVE_COMPONENTS_WEB_MCP_CORE_BROWSER_WEB_MCP_COMPONENT_INSTALLER_H_
