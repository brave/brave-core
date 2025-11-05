/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_UPDATE_CLIENT_PRIVACY_PRESERVING_PROTOCOL_SERIALIZER_H_
#define BRAVE_COMPONENTS_UPDATE_CLIENT_PRIVACY_PRESERVING_PROTOCOL_SERIALIZER_H_

#include <string>

#include "components/update_client/protocol_definition.h"
#include "components/update_client/protocol_serializer.h"

namespace update_client {
// PrivacyPreservingProtocolSerializer wraps around upstream's
// ProtocolSerializerJSON, removing values from update requests that could be
// used to fingerprint users.
class PrivacyPreservingProtocolSerializer : public ProtocolSerializer {
 public:
  PrivacyPreservingProtocolSerializer() = default;

  PrivacyPreservingProtocolSerializer(
      const PrivacyPreservingProtocolSerializer&) = delete;
  PrivacyPreservingProtocolSerializer& operator=(
      const PrivacyPreservingProtocolSerializer&) = delete;

  // Overrides for ProtocolSerializer.
  std::string Serialize(
      const protocol_request::Request& request) const override;
};

}  // namespace update_client

#endif  // BRAVE_COMPONENTS_UPDATE_CLIENT_PRIVACY_PRESERVING_PROTOCOL_SERIALIZER_H_
