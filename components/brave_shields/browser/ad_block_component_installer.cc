/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/ad_block_component_installer.h"

#include <memory>
#include <string>
#include <vector>

#include "base/base64.h"
#include "base/bind.h"
#include "base/callback.h"
#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "components/component_updater/component_installer.h"
#include "components/component_updater/component_updater_service.h"
#include "crypto/sha2.h"

using brave_component_updater::BraveOnDemandUpdater;

namespace brave_shields {

namespace {

constexpr size_t kHashSize = 32;
const char kAdBlockResourceComponentName[] = "Brave Ad Block Resources Library";
const char kAdBlockResourceComponentId[] = "mfddibmblmbccpadfndgakiopmmhebop";
const char kAdBlockResourceComponentBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA7Qk6xtml8Siq8RD6cCbd"
    "JpArt0kMci82W/KYw3KR96y67MZAsKJa8rOV2WC1BIpW539Qgl5b5lMS04cjw+sS"
    "B7f2ZKM1WOqKNij24nvEKVubunP32u8tbjtzQk9VYNcM2MZMs330eqk7iuBRTvRV"
    "iSMSeE3ymqp03HFpUGsdtjEBh1A5lroCg41eVnMn1I4GKPvuhT/Qc9Yem5gzXT/3"
    "n7H6vOGQ2dVBHz44mhgwtiDcsduh+Det6lCE2TgHOhHPdCewklgcoiNXP4zfXxfp"
    "Py1jbwb4w5KUnHSRelhfDnt+jI3jgHsD4IXdVNE5H5ZAnmcOJttbkRiT8kOVS0rJ"
    "XwIDAQAB";

const char kAdBlockFilterListCatalogComponentName[] =
    "Brave Ad Block List Catalog";
const char kAdBlockFilterListCatalogComponentId[] =
    "gkboaolpopklhgplhaaiboijnklogmbc";
const char kAdBlockFilterListCatalogComponentBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAsAnb1lw5UA1Ww4JIVE8P"
    "jKNlPogAdFoie+Aczk6ppQ4OrHANxz6oAk1xFuT2W3uhGOc3b/1ydIUMqOIdRFvM"
    "dEDUvKVeFyNAVXNSouFF7EBLEzcZfFtqoxeIbwEplVISUm+WUbsdVB9MInY3a4O3"
    "kNNuUijY7bmHzAqWMTrBfenw0Lqv38OfREXCiNq/+Jm/gt7FhyBd2oviXWEGp6as"
    "UwNavFnj8gQDGVvCf+dse8HRMJn00QH0MOypsZSWFZRmF08ybOu/jTiUo/TuIaHL"
    "1H8y9SR970LqsUMozu3ioSHtFh/IVgq7Nqy4TljaKsTE+3AdtjiOyHpW9ZaOkA7j"
    "2QIDAQAB";

const char kAdBlockIosDefaultDatComponentName[] = "Brave Ad Block Updater";
const char kAdBlockIosDefaultDatComponentId[] =
    "cffkpbalmllkdoenhmdmpbkajipdjfam";
const char kAdBlockIosDefaultDatComponentBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAs0qzJmHSgIiw7IGFCxij"
    "1NnB5hJ5ZQ1LKW9htL4EBOaMJvmqaDs/wfq0nw/goBHWsqqkMBynRTu2Hxxirvdb"
    "cugn1Goys5QKPgAvKwDHJp9jlnADWm5xQvPQ4GE1mK1/I3ka9cEOCzPW6GI+wGLi"
    "VPx9VZrxHHsSBIJRaEB5Tyi5bj0CZ+kcfMnRTsXIBw3C6xJgCVKISQUkd8mawVvG"
    "vqOhBOogCdb9qza5eJ1Cgx8RWKucFfaWWxKLOelCiBMT1Hm1znAoVBHG/blhJJOD"
    "5HcH/heRrB4MvrE1J76WF3fvZ03aHVcnlLtQeiNNOZ7VbBDXdie8Nomf/QswbBGa"
    "VwIDAQAB";

class AdBlockComponentInstallerPolicy
    : public component_updater::ComponentInstallerPolicy {
 public:
  explicit AdBlockComponentInstallerPolicy(
      const std::string& component_public_key,
      const std::string& component_id,
      const std::string& component_name,
      OnComponentReadyCallback callback);
  ~AdBlockComponentInstallerPolicy() override;

  AdBlockComponentInstallerPolicy(const AdBlockComponentInstallerPolicy&) =
      delete;
  AdBlockComponentInstallerPolicy& operator=(
      const AdBlockComponentInstallerPolicy&) = delete;

  // component_updater::ComponentInstallerPolicy
  bool SupportsGroupPolicyEnabledComponentUpdates() const override;
  bool RequiresNetworkEncryption() const override;
  update_client::CrxInstaller::Result OnCustomInstall(
      const base::Value& manifest,
      const base::FilePath& install_dir) override;
  void OnCustomUninstall() override;
  bool VerifyInstallation(const base::Value& manifest,
                          const base::FilePath& install_dir) const override;
  void ComponentReady(const base::Version& version,
                      const base::FilePath& path,
                      base::Value manifest) override;
  base::FilePath GetRelativeInstallDir() const override;
  void GetHash(std::vector<uint8_t>* hash) const override;
  std::string GetName() const override;
  update_client::InstallerAttributes GetInstallerAttributes() const override;

 private:
  const std::string component_id_;
  const std::string component_name_;
  OnComponentReadyCallback ready_callback_;
  uint8_t component_hash_[kHashSize];
};

AdBlockComponentInstallerPolicy::AdBlockComponentInstallerPolicy(
    const std::string& component_public_key,
    const std::string& component_id,
    const std::string& component_name,
    OnComponentReadyCallback callback)
    : component_id_(component_id),
      component_name_(component_name),
      ready_callback_(callback) {
  // Generate hash from public key.
  std::string decoded_public_key;
  base::Base64Decode(component_public_key, &decoded_public_key);
  crypto::SHA256HashString(decoded_public_key, component_hash_, kHashSize);
}

AdBlockComponentInstallerPolicy::~AdBlockComponentInstallerPolicy() = default;

bool AdBlockComponentInstallerPolicy::
    SupportsGroupPolicyEnabledComponentUpdates() const {
  return true;
}

bool AdBlockComponentInstallerPolicy::RequiresNetworkEncryption() const {
  return false;
}

update_client::CrxInstaller::Result
AdBlockComponentInstallerPolicy::OnCustomInstall(
    const base::Value& manifest,
    const base::FilePath& install_dir) {
  return update_client::CrxInstaller::Result(0);
}

void AdBlockComponentInstallerPolicy::OnCustomUninstall() {}

void AdBlockComponentInstallerPolicy::ComponentReady(
    const base::Version& version,
    const base::FilePath& path,
    base::Value manifest) {
  ready_callback_.Run(path);
}

bool AdBlockComponentInstallerPolicy::VerifyInstallation(
    const base::Value& manifest,
    const base::FilePath& install_dir) const {
  return true;
}

base::FilePath AdBlockComponentInstallerPolicy::GetRelativeInstallDir() const {
  return base::FilePath::FromUTF8Unsafe(component_id_);
}

void AdBlockComponentInstallerPolicy::GetHash(
    std::vector<uint8_t>* hash) const {
  hash->assign(component_hash_, component_hash_ + kHashSize);
}

std::string AdBlockComponentInstallerPolicy::GetName() const {
  return component_name_;
}

update_client::InstallerAttributes
AdBlockComponentInstallerPolicy::GetInstallerAttributes() const {
  return update_client::InstallerAttributes();
}

void OnRegistered(const std::string& component_id) {
  BraveOnDemandUpdater::GetInstance()->OnDemandUpdate(component_id);
}

}  // namespace

void RegisterAdBlockIosDefaultDatComponent(
    component_updater::ComponentUpdateService* cus,
    OnComponentReadyCallback callback) {
  // In test, |cus| could be nullptr.
  if (!cus)
    return;

  auto installer = base::MakeRefCounted<component_updater::ComponentInstaller>(
      std::make_unique<AdBlockComponentInstallerPolicy>(
          kAdBlockIosDefaultDatComponentBase64PublicKey,
          kAdBlockIosDefaultDatComponentId, kAdBlockIosDefaultDatComponentName,
          callback));
  installer->Register(
      cus, base::BindOnce(&OnRegistered, kAdBlockIosDefaultDatComponentId));
}

void RegisterAdBlockDefaultResourceComponent(
    component_updater::ComponentUpdateService* cus,
    OnComponentReadyCallback callback) {
  // In test, |cus| could be nullptr.
  if (!cus)
    return;

  auto installer = base::MakeRefCounted<component_updater::ComponentInstaller>(
      std::make_unique<AdBlockComponentInstallerPolicy>(
          kAdBlockResourceComponentBase64PublicKey, kAdBlockResourceComponentId,
          kAdBlockResourceComponentName, callback));
  installer->Register(
      cus, base::BindOnce(&OnRegistered, kAdBlockResourceComponentId));
}

void RegisterAdBlockFilterListCatalogComponent(
    component_updater::ComponentUpdateService* cus,
    OnComponentReadyCallback callback) {
  // In test, |cus| could be nullptr.
  if (!cus)
    return;

  auto installer = base::MakeRefCounted<component_updater::ComponentInstaller>(
      std::make_unique<AdBlockComponentInstallerPolicy>(
          kAdBlockFilterListCatalogComponentBase64PublicKey,
          kAdBlockFilterListCatalogComponentId,
          kAdBlockFilterListCatalogComponentName, callback));
  installer->Register(
      cus, base::BindOnce(&OnRegistered, kAdBlockFilterListCatalogComponentId));
}

void RegisterAdBlockFiltersComponent(
    component_updater::ComponentUpdateService* cus,
    const std::string& component_public_key,
    const std::string& component_id,
    const std::string& component_name,
    OnComponentReadyCallback callback) {
  // In test, |cus| could be nullptr.
  if (!cus)
    return;

  auto installer = base::MakeRefCounted<component_updater::ComponentInstaller>(
      std::make_unique<AdBlockComponentInstallerPolicy>(
          component_public_key, component_id, component_name, callback));
  installer->Register(cus, base::BindOnce(&OnRegistered, component_id));
}

}  // namespace brave_shields
