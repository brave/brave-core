/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/update_client/privacy_preserving_protocol_handler.h"

#include <memory>

#include "brave/components/update_client/privacy_preserving_protocol_serializer.h"
#include "components/update_client/protocol_handler.h"
#include "components/update_client/protocol_parser.h"

namespace update_client {

std::unique_ptr<ProtocolParser>
PrivacyPreservingProtocolHandlerFactory::CreateParser() const {
  // We're not interested in changing this behavior. Mirror upstream.
  return ProtocolHandlerFactoryJSON().CreateParser();
}

std::unique_ptr<ProtocolSerializer>
PrivacyPreservingProtocolHandlerFactory::CreateSerializer() const {
  return std::make_unique<PrivacyPreservingProtocolSerializer>();
}

}  // namespace update_client
