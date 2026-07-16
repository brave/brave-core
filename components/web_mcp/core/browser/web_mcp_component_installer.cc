// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/web_mcp/core/browser/web_mcp_component_installer.h"

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "base/base64.h"
#include "base/check.h"
#include "base/containers/to_vector.h"
#include "base/functional/bind.h"
#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "brave/components/web_mcp/core/browser/web_mcp_rule_registry.h"
#include "components/component_updater/component_installer.h"
#include "components/component_updater/component_updater_service.h"
#include "crypto/sha2.h"

namespace web_mcp {

// Directory structure of the WebMCP component:
// eingdhelnaolbpcdkgddekhifcjfkalf/<component version>/
//  |_ manifest.json
//  |_ scripts/
//    |_ gmail_unread_count.js
//    |_ example_page_heading.js
// Each script holds its tool metadata in a `// ==WebMCP==` comment block; see
// web_mcp_injection_rule.h for the format.

namespace {

class WebMcpComponentInstallerPolicy
    : public component_updater::ComponentInstallerPolicy {
 public:
  WebMcpComponentInstallerPolicy();

  WebMcpComponentInstallerPolicy(const WebMcpComponentInstallerPolicy&) =
      delete;
  WebMcpComponentInstallerPolicy& operator=(
      const WebMcpComponentInstallerPolicy&) = delete;

  // component_updater::ComponentInstallerPolicy:
  bool SupportsGroupPolicyEnabledComponentUpdates() const override;
  bool RequiresNetworkEncryption() const override;
  update_client::CrxInstaller::Result OnCustomInstall(
      const base::DictValue& manifest,
      const base::FilePath& install_dir) override;
  void OnCustomUninstall() override;
  bool VerifyInstallation(const base::DictValue& manifest,
                          const base::FilePath& install_dir) const override;
  void ComponentReady(const base::Version& version,
                      const base::FilePath& path,
                      base::DictValue manifest) override;
  base::FilePath GetRelativeInstallDir() const override;
  void GetHash(std::vector<uint8_t>* hash) const override;
  std::string GetName() const override;
  update_client::InstallerAttributes GetInstallerAttributes() const override;
  bool IsBraveComponent() const override;

 private:
  std::array<uint8_t, crypto::kSHA256Length> component_hash_;
};

WebMcpComponentInstallerPolicy::WebMcpComponentInstallerPolicy() {
  // Generate hash from public key.
  auto decoded_public_key = base::Base64Decode(kWebMcpComponentBase64PublicKey);
  CHECK(decoded_public_key);
  component_hash_ = crypto::SHA256Hash(*decoded_public_key);
}

bool WebMcpComponentInstallerPolicy::
    SupportsGroupPolicyEnabledComponentUpdates() const {
  return true;
}

bool WebMcpComponentInstallerPolicy::RequiresNetworkEncryption() const {
  return false;
}

update_client::CrxInstaller::Result
WebMcpComponentInstallerPolicy::OnCustomInstall(
    const base::DictValue& manifest,
    const base::FilePath& install_dir) {
  return update_client::CrxInstaller::Result(0);
}

void WebMcpComponentInstallerPolicy::OnCustomUninstall() {}

bool WebMcpComponentInstallerPolicy::VerifyInstallation(
    const base::DictValue& manifest,
    const base::FilePath& install_dir) const {
  return true;
}

void WebMcpComponentInstallerPolicy::ComponentReady(
    const base::Version& version,
    const base::FilePath& install_dir,
    base::DictValue manifest) {
  WebMcpRuleRegistry::GetInstance()->LoadRules(install_dir);
}

base::FilePath WebMcpComponentInstallerPolicy::GetRelativeInstallDir() const {
  return base::FilePath::FromUTF8Unsafe(kWebMcpComponentId);
}

void WebMcpComponentInstallerPolicy::GetHash(std::vector<uint8_t>* hash) const {
  *hash = base::ToVector(component_hash_);
}

std::string WebMcpComponentInstallerPolicy::GetName() const {
  return kWebMcpComponentName;
}

update_client::InstallerAttributes
WebMcpComponentInstallerPolicy::GetInstallerAttributes() const {
  return update_client::InstallerAttributes();
}

bool WebMcpComponentInstallerPolicy::IsBraveComponent() const {
  return true;
}

}  // namespace

void RegisterWebMcpComponent(component_updater::ComponentUpdateService* cus) {
  if (!cus) {
    return;
  }

  auto installer = base::MakeRefCounted<component_updater::ComponentInstaller>(
      std::make_unique<WebMcpComponentInstallerPolicy>());
  installer->Register(
      // After Register, ensure the component is installed.
      cus, base::BindOnce([]() {
        brave_component_updater::BraveOnDemandUpdater::GetInstance()
            ->EnsureInstalled(kWebMcpComponentId);
      }));
}

}  // namespace web_mcp
