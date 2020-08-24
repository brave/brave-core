/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/ipfs_api.h"

#include <memory>
#include <string>

#include "base/json/json_writer.h"
#include "base/values.h"
#include "brave/browser/ipfs/ipfs_service.h"
#include "brave/browser/ipfs/ipfs_service_factory.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/common/extensions/extension_constants.h"
#include "brave/common/pref_names.h"
#include "brave/components/ipfs/browser/brave_ipfs_client_updater.h"
#include "brave/components/ipfs/browser/features.h"
#include "brave/components/ipfs/common/ipfs_constants.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/extensions/api/tabs/tabs_constants.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/extension_util.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

ipfs::IpfsService* GetIPFSService(
    content::BrowserContext* context) {
  return ipfs::IpfsServiceFactory::GetInstance()
      ->GetForContext(Profile::FromBrowserContext(context));
}

base::Value MakeSelectValue(const  base::string16& name,
                            ipfs::IPFSResolveMethodTypes value) {
  base::Value item(base::Value::Type::DICTIONARY);
  item.SetKey("value", base::Value(static_cast<int>(value)));
  item.SetKey("name", base::Value(name));
  return item;
}

}  // namespace

namespace extensions {
namespace api {

ExtensionFunction::ResponseAction
IpfsGetIPFSResolveMethodListFunction::Run() {
  base::Value list(base::Value::Type::LIST);
  list.Append(MakeSelectValue(
      l10n_util::GetStringUTF16(IDS_IPFS_RESOLVE_OPTION_GATEWAY),
      ipfs::IPFSResolveMethodTypes::IPFS_GATEWAY));

  if (GetIPFSService(browser_context())->IsIPFSExecutableAvailable()) {
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

ExtensionFunction::ResponseAction
IpfsGetIPFSEnabledFunction::Run() {
  bool enabled = base::FeatureList::IsEnabled(ipfs::features::kIpfsFeature);
  return RespondNow(OneArgument(std::make_unique<base::Value>(enabled)));
}

}  // namespace api
}  // namespace extensions
