/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_API_IPFS_API_H_
#define BRAVE_BROWSER_EXTENSIONS_API_IPFS_API_H_

#include <string>

#include "extensions/browser/extension_function.h"

class Profile;

namespace ipfs {
class IpnsKeysManager;
}  // namespace ipfs

namespace extensions {
namespace api {

class IpfsGetIpfsPeersListFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("ipfs.getIpfsPeersList", UNKNOWN)

 protected:
  void OnConfigLoaded(bool success, const std::string& config);
  ~IpfsGetIpfsPeersListFunction() override {}
  ResponseAction Run() override;
};

class IpfsAddIpfsPeerFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("ipfs.addIpfsPeer", UNKNOWN)

 protected:
  void OnPeerAdded(bool result, const std::string& value);
  void OnConfigLoaded(const std::string& peer,
                      bool success,
                      const std::string& config);
  void OnConfigUpdated(bool success);

  ~IpfsAddIpfsPeerFunction() override {}
  ResponseAction Run() override;
};

class IpfsRemoveIpfsPeerFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("ipfs.removeIpfsPeer", UNKNOWN)

 protected:
  void OnPeerAdded(bool result, const std::string& value);
  void OnConfigLoaded(const std::string& peer_id,
                      const std::string& address,
                      bool success,
                      const std::string& config);
  void OnConfigUpdated(bool success);

  ~IpfsRemoveIpfsPeerFunction() override {}
  ResponseAction Run() override;
};

class IpfsRemoveIpnsKeyFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("ipfs.removeIpnsKey", UNKNOWN)

 protected:
  void OnKeyRemoved(::ipfs::IpnsKeysManager* manager, const std::string&, bool);

  ~IpfsRemoveIpnsKeyFunction() override {}
  ResponseAction Run() override;
};

class IpfsGetResolveMethodListFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("ipfs.getResolveMethodList", UNKNOWN)

 protected:
  ~IpfsGetResolveMethodListFunction() override {}
  ResponseAction Run() override;
};

class IpfsRotateKeyFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("ipfs.rotateKey", UNKNOWN)

 protected:
  void OnKeyRotated(bool result);

  ~IpfsRotateKeyFunction() override {}
  ResponseAction Run() override;
};

class IpfsAddIpnsKeyFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("ipfs.addIpnsKey", UNKNOWN)

 protected:
  void OnKeyCreated(::ipfs::IpnsKeysManager* manager,
                    bool result,
                    const std::string& name,
                    const std::string& value);

  ~IpfsAddIpnsKeyFunction() override {}
  ResponseAction Run() override;
};

class IpfsGetIpnsKeysListFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("ipfs.getIpnsKeysList", UNKNOWN)

 protected:
  void OnKeysLoaded(::ipfs::IpnsKeysManager* manager, bool success);

  ~IpfsGetIpnsKeysListFunction() override {}
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

class IpfsValidateGatewayUrlFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("ipfs.validateGatewayUrl", UNKNOWN)

 protected:
  void OnGatewayValidated(bool success);

  ~IpfsValidateGatewayUrlFunction() override {}
  ResponseAction Run() override;
};

}  // namespace api
}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_API_IPFS_API_H_
