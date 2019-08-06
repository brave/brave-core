/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/internal/confirmation_info.h"

namespace confirmations {

ConfirmationInfo::ConfirmationInfo() :
    id(""),
    creative_instance_id(""),
    type(ConfirmationType::UNKNOWN),
    token_info(TokenInfo()),
    payment_token(nullptr),
    blinded_payment_token(nullptr),
    credential(""),
    timestamp_in_seconds(0),
    created(false) {}

ConfirmationInfo::ConfirmationInfo(const ConfirmationInfo& info) :
    id(info.id),
    creative_instance_id(info.creative_instance_id),
    type(info.type),
    token_info(info.token_info),
    payment_token(info.payment_token),
    blinded_payment_token(info.blinded_payment_token),
    credential(info.credential),
    timestamp_in_seconds(info.timestamp_in_seconds),
    created(info.created) {}

ConfirmationInfo::~ConfirmationInfo() {}

}  // namespace confirmations
