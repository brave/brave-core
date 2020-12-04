/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/ipfs_api.h"

#include <memory>
#include <string>

#include "base/json/json_writer.h"
#include "base/values.h"
#include "brave/browser/ipfs/ipfs_service_factory.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_service.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "brave/grit/brave_generated_resources.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

ipfs::IpfsService* GetIpfsService(content::BrowserContext* context) {
  return ipfs::IpfsServiceFactory::GetInstance()->GetForContext(context);
}

bool IsIpfsEnabled(content::BrowserContext* context) {
  return ipfs::IsIpfsEnabled(context);
}

base::Value MakeSelectValue(const base::string16& name,
                            ipfs::IPFSResolveMethodTypes value) {
  base::Value item(base::Value::Type::DICTIONARY);
  item.SetKey("value", base::Value(static_cast<int>(value)));
  item.SetKey("name", base::Value(name));
  return item;
}

}  // namespace

namespace extensions {
namespace api {

ExtensionFunction::ResponseAction IpfsGetResolveMethodListFunction::Run() {
  base::Value list(base::Value::Type::LIST);
  list.Append(
      MakeSelectValue(l10n_util::GetStringUTF16(IDS_IPFS_RESOLVE_OPTION_ASK),
                      ipfs::IPFSResolveMethodTypes::IPFS_ASK));
  list.Append(MakeSelectValue(
      l10n_util::GetStringUTF16(IDS_IPFS_RESOLVE_OPTION_GATEWAY),
      ipfs::IPFSResolveMethodTypes::IPFS_GATEWAY));

  if (GetIpfsService(browser_context()) &&
      GetIpfsService(browser_context())->IsIPFSExecutableAvailable()) {
    list.Append(MakeSelectValue(
        l10n_util::GetStringUTF16(IDS_IPFS_RESOLVE_OPTION_LOCAL),
        ipfs::IPFSResolveMethodTypes::IPFS_LOCAL));
  }
  list.Append(MakeSelectValue(
      l10n_util::GetStringUTF16(IDS_IPFS_RESOLVE_OPTION_DISABLED),
      ipfs::IPFSResolveMethodTypes::IPFS_DISABLED));
  std::string json_string;
  base::JSONWriter::Write(list, &json_string);
  return RespondNow(OneArgument(std::make_unique<base::Value>(json_string)));
}

ExtensionFunction::ResponseAction IpfsGetIPFSEnabledFunction::Run() {
  bool enabled = IsIpfsEnabled(browser_context());
  return RespondNow(OneArgument(std::make_unique<base::Value>(enabled)));
}

ExtensionFunction::ResponseAction IpfsGetResolveMethodTypeFunction::Run() {
  std::string value = "invalid";
  if (IsIpfsEnabled(browser_context())) {
    switch (GetIpfsService(browser_context())->GetIPFSResolveMethodType()) {
      case ipfs::IPFSResolveMethodTypes::IPFS_ASK:
        value = "ask";
        break;
      case ipfs::IPFSResolveMethodTypes::IPFS_GATEWAY:
        value = "gateway";
        break;
      case ipfs::IPFSResolveMethodTypes::IPFS_LOCAL:
        value = "local";
        break;
      case ipfs::IPFSResolveMethodTypes::IPFS_DISABLED:
        value = "disabled";
        break;
    }
  }
  return RespondNow(OneArgument(std::make_unique<base::Value>(value)));
}

ExtensionFunction::ResponseAction IpfsLaunchFunction::Run() {
  if (!IsIpfsEnabled(browser_context())) {
    return RespondNow(Error("IPFS not enabled"));
  }
  GetIpfsService(browser_context())
      ->LaunchDaemon(base::BindOnce(&IpfsLaunchFunction::OnLaunch, this));
  return RespondLater();
}

void IpfsLaunchFunction::OnLaunch(bool launched) {
  Respond(OneArgument(std::make_unique<base::Value>(launched)));
}

ExtensionFunction::ResponseAction IpfsShutdownFunction::Run() {
  if (!IsIpfsEnabled(browser_context())) {
    return RespondNow(Error("IPFS not enabled"));
  }
  GetIpfsService(browser_context())
      ->ShutdownDaemon(base::BindOnce(&IpfsShutdownFunction::OnShutdown, this));
  return RespondLater();
}

void IpfsShutdownFunction::OnShutdown(bool shutdown) {
  Respond(OneArgument(std::make_unique<base::Value>(shutdown)));
}

ExtensionFunction::ResponseAction IpfsGetConfigFunction::Run() {
  if (!IsIpfsEnabled(browser_context())) {
    return RespondNow(Error("IPFS not enabled"));
  }
  GetIpfsService(browser_context())
      ->GetConfig(base::BindOnce(&IpfsGetConfigFunction::OnGetConfig, this));
  return RespondLater();
}

void IpfsGetConfigFunction::OnGetConfig(bool success,
                                        const std::string& value) {
  Respond(TwoArguments(std::make_unique<base::Value>(success),
                       std::make_unique<base::Value>(value)));
}

ExtensionFunction::ResponseAction IpfsGetExecutableAvailableFunction::Run() {
  if (!IsIpfsEnabled(browser_context())) {
    return RespondNow(Error("IPFS not enabled"));
  }
  bool avail = GetIpfsService(browser_context())->IsIPFSExecutableAvailable();
  return RespondNow(OneArgument(std::make_unique<base::Value>(avail)));
}

}  // namespace api
}  // namespace extensions
