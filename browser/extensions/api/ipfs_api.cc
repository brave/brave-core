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
#include "brave/components/ipfs/keys/ipns_keys_manager.h"
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

base::Value MakeValue(const std::string& name, const std::string& value) {
  base::Value item(base::Value::Type::DICTIONARY);
  item.SetKey("value", base::Value(value));
  item.SetKey("name", base::Value(name));
  return item;
}

base::Value MakeResponseFromMap(const ipfs::IpnsKeysManager::KeysMap& keys) {
  base::Value list(base::Value::Type::LIST);
  for (const auto& key : keys) {
    list.Append(MakeValue(key.first, key.second));
  }
  std::string json_string;
  base::JSONWriter::Write(list, &json_string);
  return base::Value(json_string);
}
}  // namespace

namespace extensions {
namespace api {

ExtensionFunction::ResponseAction IpfsRemoveIpnsKeyFunction::Run() {
  if (!IsIpfsEnabled(browser_context()))
    return RespondNow(Error("IPFS not enabled"));
  ::ipfs::IpfsService* ipfs_service = GetIpfsService(browser_context());
  if (!ipfs_service) {
    return RespondNow(Error("Could not obtain IPFS service"));
  }
  ::ipfs::IpnsKeysManager* key_manager = ipfs_service->GetIpnsKeysManager();
  if (!ipfs_service->IsDaemonLaunched() || !key_manager) {
    return RespondNow(Error("IPFS node is not launched"));
  }
  std::unique_ptr<ipfs::RemoveIpnsKey::Params> params(
      ipfs::RemoveIpnsKey::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());
  key_manager->RemoveKey(
      params->name, base::BindOnce(&IpfsRemoveIpnsKeyFunction::OnKeyRemoved,
                                   base::RetainedRef(this), key_manager));
  return RespondLater();
}

void IpfsRemoveIpnsKeyFunction::OnKeyRemoved(::ipfs::IpnsKeysManager* manager,
                                             const std::string& name,
                                             bool success) {
  DCHECK(manager);
  if (!success) {
    return Respond(Error("Unable to remove key"));
  }
  return Respond(OneArgument(base::Value(name)));
}

ExtensionFunction::ResponseAction IpfsAddIpnsKeyFunction::Run() {
  if (!IsIpfsEnabled(browser_context()))
    return RespondNow(Error("IPFS not enabled"));
  ::ipfs::IpfsService* ipfs_service = GetIpfsService(browser_context());
  if (!ipfs_service) {
    return RespondNow(Error("Could not obtain IPFS service"));
  }
  ::ipfs::IpnsKeysManager* key_manager = ipfs_service->GetIpnsKeysManager();
  if (!ipfs_service->IsDaemonLaunched() || !key_manager) {
    return RespondNow(Error("IPFS node is not launched"));
  }
  std::unique_ptr<ipfs::AddIpnsKey::Params> params(
      ipfs::AddIpnsKey::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());
  key_manager->GenerateNewKey(
      params->name, base::BindOnce(&IpfsAddIpnsKeyFunction::OnKeyCreated,
                                   base::RetainedRef(this), key_manager));
  return RespondLater();
}

void IpfsAddIpnsKeyFunction::OnKeyCreated(::ipfs::IpnsKeysManager* manager,
                                          bool success,
                                          const std::string& name,
                                          const std::string& value) {
  DCHECK(manager);
  if (!success) {
    return Respond(Error("Unable to create key"));
  }
  std::string json_string;
  base::JSONWriter::Write(MakeValue(name, value), &json_string);
  return Respond(OneArgument(base::Value(json_string)));
}

ExtensionFunction::ResponseAction IpfsGetIpnsKeysListFunction::Run() {
  if (!IsIpfsEnabled(browser_context()))
    return RespondNow(Error("IPFS not enabled"));
  ::ipfs::IpfsService* ipfs_service = GetIpfsService(browser_context());
  if (!ipfs_service) {
    return RespondNow(Error("Could not obtain IPFS service"));
  }
  ::ipfs::IpnsKeysManager* key_manager = ipfs_service->GetIpnsKeysManager();
  if (!ipfs_service->IsDaemonLaunched() || !key_manager) {
    return RespondNow(Error("IPFS node is not launched"));
  }
  const auto& keys = key_manager->GetKeys();
  if (!keys.size()) {
    key_manager->LoadKeys(
        base::BindOnce(&IpfsGetIpnsKeysListFunction::OnKeysLoaded,
                       base::RetainedRef(this), key_manager));
    return RespondLater();
  }
  return RespondNow(OneArgument(MakeResponseFromMap(keys)));
}

void IpfsGetIpnsKeysListFunction::OnKeysLoaded(::ipfs::IpnsKeysManager* manager,
                                               bool success) {
  DCHECK(manager);
  if (!success) {
    return Respond(Error("Unable to load keys"));
  }
  return Respond(OneArgument(MakeResponseFromMap(manager->GetKeys())));
}

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
      ->LaunchDaemon(base::BindOnce(&IpfsLaunchFunction::OnLaunch,
                                    base::RetainedRef(this)));
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
      ->ShutdownDaemon(base::BindOnce(&IpfsShutdownFunction::OnShutdown,
                                      base::RetainedRef(this)));
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
      ->GetConfig(base::BindOnce(&IpfsGetConfigFunction::OnGetConfig,
                                 base::RetainedRef(this)));
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
