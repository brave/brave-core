/* Copyright (c) 2020 The Brave Authors. All rights reserved.
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

class IpfsGetResolveMethodListFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("ipfs.getResolveMethodList", UNKNOWN)

 protected:
  ~IpfsGetResolveMethodListFunction() override {}
  ResponseAction Run() override;
};

class IpfsGetIPFSEnabledFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("ipfs.getIPFSEnabled", UNKNOWN)

 protected:
  ~IpfsGetIPFSEnabledFunction() override {}
  ResponseAction Run() override;
};

class IpfsGetResolveMethodTypeFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("ipfs.getResolveMethodType", UNKNOWN)

 protected:
  ~IpfsGetResolveMethodTypeFunction() override {}
  ResponseAction Run() override;
};

class IpfsLaunchFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("ipfs.launch", UNKNOWN)

 protected:
  void OnLaunch(bool);
  ~IpfsLaunchFunction() override {}
  ResponseAction Run() override;
};

class IpfsShutdownFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("ipfs.shutdown", UNKNOWN)

 protected:
  void OnShutdown(bool);
  ~IpfsShutdownFunction() override {}
  ResponseAction Run() override;
};

class IpfsGetConfigFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("ipfs.getConfig", UNKNOWN)

 protected:
  void OnGetConfig(bool, const std::string& config);
  ~IpfsGetConfigFunction() override {}
  ResponseAction Run() override;
};

class IpfsGetExecutableAvailableFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("ipfs.getExecutableAvailable", UNKNOWN)

 protected:
  ~IpfsGetExecutableAvailableFunction() override {}
  ResponseAction Run() override;
};

class IpfsResolveIPFSURIFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("ipfs.resolveIPFSURI", UNKNOWN)

 protected:
  ~IpfsResolveIPFSURIFunction() override {}
  ResponseAction Run() override;
};

}  // namespace api
}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_API_IPFS_API_H_
