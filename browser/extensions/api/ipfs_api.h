/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_API_IPFS_API_H_
#define BRAVE_BROWSER_EXTENSIONS_API_IPFS_API_H_

#include <string>

#include "extensions/browser/extension_function.h"

class Profile;

namespace extensions {
namespace api {

class IpfsGetIPFSResolveMethodListFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("ipfs.getIPFSResolveMethodList", UNKNOWN)

 protected:
  ~IpfsGetIPFSResolveMethodListFunction() override {}
  ResponseAction Run() override;
};

class IpfsGetIPFSEnabledFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("ipfs.getIPFSEnabled", UNKNOWN)

 protected:
  ~IpfsGetIPFSEnabledFunction() override {}
  ResponseAction Run() override;
};

}  // namespace api
}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_API_IPFS_API_H_
