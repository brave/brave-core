/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/issuer_info.h"

namespace confirmations {

IssuerInfo::IssuerInfo() : name(""), public_key("") {}

IssuerInfo::IssuerInfo(const IssuerInfo& info)
    : name(info.name), public_key(info.public_key) {}

IssuerInfo::~IssuerInfo() = default;

}  // namespace confirmations
