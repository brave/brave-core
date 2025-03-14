// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_USER_AGENT_BROWSER_BRAVE_USER_AGENT_COMPONENT_INSTALLER_H_
#define BRAVE_COMPONENTS_BRAVE_USER_AGENT_BROWSER_BRAVE_USER_AGENT_COMPONENT_INSTALLER_H_

#include "base/files/file_path.h"

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace brave_user_agent {

inline constexpr char kBraveUserAgentServiceComponentName[] =
    "Brave User Agent Service";
inline constexpr char kBraveUserAgentServiceComponentId[] =
    "nlpaeekllejnmhoonlpcefpfnpbajbpe";
inline constexpr char kBraveUserAgentServiceComponentBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAnZM5zlosFqJ+SJ85K1+"
    "5yZzuvTkxKDRQ3o+MBRCmRpIrT4DhYuhY89X+"
    "0DsgsbVjMOr8V3GeAzZZJ11JvQWjk7IlMAaRjx4HrWDDx7AlOObXNlpco6E2vuqVIMDsw1Tbha"
    "AFZNqs5M0vUUgxv99IbGD2Db6l2fQ4crz01OwcFK2gO9EPqFgRT33cwhlu1UnbymeV4gfR4A+"
    "oqe8tiCJBZN1n0usDuOV2/"
    "xPc2QAJxCPk4AMVZ3bi0N0GjVwyrdPHiuPttniF83fxpjGQG2aZFDfRv8IkX0VJ9pYXkIcZac1"
    "Gpo8vsnG7fHSm6NN/g7LdJuG7NMRUFM6dzgK1HwWyEwIDAQAB";

// Registers the BraveUserAgent component with the component updater.
void RegisterBraveUserAgentComponent(
    component_updater::ComponentUpdateService* cus);

}  // namespace brave_user_agent

#endif  // BRAVE_COMPONENTS_BRAVE_USER_AGENT_BROWSER_BRAVE_USER_AGENT_COMPONENT_INSTALLER_H_
