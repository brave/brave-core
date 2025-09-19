/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/update_client/privacy_preserving_protocol_handler.h"

#include <memory>

#include "brave/components/update_client/brave_protocol_serializer_json.h"
#include "components/update_client/protocol_handler.h"
#include "components/update_client/protocol_parser.h"

namespace update_client {

std::unique_ptr<ProtocolParser> BraveProtocolHandlerFactoryJSON::CreateParser()
    const {
  // We're not interested in changing this behavior. Mirror upstream.
  ProtocolHandlerFactoryJSON upstream_factory;
  return upstream_factory.CreateParser();
}

std::unique_ptr<ProtocolSerializer>
BraveProtocolHandlerFactoryJSON::CreateSerializer() const {
  return std::make_unique<BraveProtocolSerializerJSON>();
}

}  // namespace update_client
