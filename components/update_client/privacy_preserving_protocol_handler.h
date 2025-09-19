/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_UPDATE_CLIENT_PRIVACY_PRESERVING_PROTOCOL_HANDLER_H_
#define BRAVE_COMPONENTS_UPDATE_CLIENT_PRIVACY_PRESERVING_PROTOCOL_HANDLER_H_

#include <memory>

#include "components/update_client/protocol_handler.h"
#include "components/update_client/protocol_parser.h"
#include "components/update_client/protocol_serializer.h"

namespace update_client {

// This class returns the same parser as upstream but returns a different
// serializer (PrivacyPreservingProtocolSerializer) to remove values from update
// requests that could be used to fingerprint users.
class PrivacyPreservingProtocolHandlerFactory : public ProtocolHandlerFactory {
 public:
  // Overrides for ProtocolHandlerFactory.
  std::unique_ptr<ProtocolParser> CreateParser() const override;
  std::unique_ptr<ProtocolSerializer> CreateSerializer() const override;
};

}  // namespace update_client

#endif  // BRAVE_COMPONENTS_UPDATE_CLIENT_PRIVACY_PRESERVING_PROTOCOL_HANDLER_H_
