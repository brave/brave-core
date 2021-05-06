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
#include "brave/common/extensions/api/ipfs.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_service.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/common/channel_info.h"
#include "ui/base/l10n/l10n_util.h"

using ipfs::IPFSResolveMethodTypes;

namespace {

ipfs::IpfsService* GetIpfsService(content::BrowserContext* context) {
  return ipfs::IpfsServiceFactory::GetInstance()->GetForContext(context);
}

bool IsIpfsEnabled(content::BrowserContext* context) {
  return ipfs::IsIpfsEnabled(context);
}

base::Value MakeSelectValue(const std::u16string& name,
                            IPFSResolveMethodTypes value) {
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
                      IPFSResolveMethodTypes::IPFS_ASK));
  list.Append(MakeSelectValue(
      l10n_util::GetStringUTF16(IDS_IPFS_RESOLVE_OPTION_GATEWAY),
      IPFSResolveMethodTypes::IPFS_GATEWAY));

  list.Append(
      MakeSelectValue(l10n_util::GetStringUTF16(IDS_IPFS_RESOLVE_OPTION_LOCAL),
                      IPFSResolveMethodTypes::IPFS_LOCAL));

  list.Append(MakeSelectValue(
      l10n_util::GetStringUTF16(IDS_IPFS_RESOLVE_OPTION_DISABLED),
      IPFSResolveMethodTypes::IPFS_DISABLED));
  std::string json_string;
  base::JSONWriter::Write(list, &json_string);
  return RespondNow(OneArgument(base::Value(json_string)));
}

ExtensionFunction::ResponseAction IpfsGetIPFSEnabledFunction::Run() {
  bool enabled = IsIpfsEnabled(browser_context());
  return RespondNow(OneArgument(base::Value(enabled)));
}

ExtensionFunction::ResponseAction IpfsGetResolveMethodTypeFunction::Run() {
  std::string value = "invalid";
  if (IsIpfsEnabled(browser_context())) {
    switch (GetIpfsService(browser_context())->GetIPFSResolveMethodType()) {
      case IPFSResolveMethodTypes::IPFS_ASK:
        value = "ask";
        break;
      case IPFSResolveMethodTypes::IPFS_GATEWAY:
        value = "gateway";
        break;
      case IPFSResolveMethodTypes::IPFS_LOCAL:
        value = "local";
        break;
      case IPFSResolveMethodTypes::IPFS_DISABLED:
        value = "disabled";
        break;
    }
  }
  return RespondNow(OneArgument(base::Value(value)));
}

ExtensionFunction::ResponseAction IpfsLaunchFunction::Run() {
  if (!IsIpfsEnabled(browser_context())) {
    return RespondNow(Error("IPFS not enabled"));
  }

  if (!GetIpfsService(browser_context())) {
    return RespondNow(Error("Could not obtain IPFS service"));
  }

  if (!GetIpfsService(browser_context())->IsIPFSExecutableAvailable()) {
    return RespondNow(OneArgument(base::Value(false)));
  }

  GetIpfsService(browser_context())
      ->LaunchDaemon(base::BindOnce(&IpfsLaunchFunction::OnLaunch, this));
  return RespondLater();
}

void IpfsLaunchFunction::OnLaunch(bool launched) {
  Respond(OneArgument(base::Value(launched)));
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
  Respond(OneArgument(base::Value(shutdown)));
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
  Respond(TwoArguments(base::Value(success), base::Value(value)));
}

ExtensionFunction::ResponseAction IpfsGetExecutableAvailableFunction::Run() {
  if (!IsIpfsEnabled(browser_context())) {
    return RespondNow(Error("IPFS not enabled"));
  }
  bool avail = GetIpfsService(browser_context())->IsIPFSExecutableAvailable();
  return RespondNow(OneArgument(base::Value(avail)));
}

ExtensionFunction::ResponseAction IpfsResolveIPFSURIFunction::Run() {
  if (!IsIpfsEnabled(browser_context())) {
    return RespondNow(Error("IPFS not enabled"));
  }

  std::unique_ptr<ipfs::ResolveIPFSURI::Params> params(
      ipfs::ResolveIPFSURI::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());
  GURL uri(params->uri);
  GURL ipfs_gateway_url;
  if (!::ipfs::ResolveIPFSURI(browser_context(), chrome::GetChannel(), uri,
                              &ipfs_gateway_url)) {
    return RespondNow(Error("Could not translate IPFS URI"));
  }

  return RespondNow(OneArgument(base::Value(ipfs_gateway_url.spec())));
}

}  // namespace api
}  // namespace extensions
