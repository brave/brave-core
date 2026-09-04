/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/flows/flow_base.h"

#include <utility>

#include "base/check.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_account {

FlowBase::FlowBase(
    AccountStatePrefs& account_state_prefs,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    const os_crypt_async::Encryptor& encryptor)
    : account_state_prefs_(account_state_prefs),
      url_loader_factory_(std::move(url_loader_factory)),
      encryption_(encryptor) {
  CHECK(url_loader_factory_);
}

FlowBase::~FlowBase() = default;

std::string FlowBase::Encrypt(const std::string& plain_text) const {
  return encryption_.Encrypt(plain_text);
}

std::string FlowBase::Decrypt(const std::string& base64) const {
  return encryption_.Decrypt(base64);
}

void FlowBase::RemoveRequestHandle(
    std::list<endpoint_client::RequestHandle>::iterator slot) {
  in_flight_.erase(slot);
}

}  // namespace brave_account
