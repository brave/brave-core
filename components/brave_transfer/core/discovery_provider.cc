// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_transfer/core/discovery_provider.h"

namespace brave_transfer {

PeerCandidate::PeerCandidate() = default;
PeerCandidate::PeerCandidate(const PeerCandidate&) = default;
PeerCandidate& PeerCandidate::operator=(const PeerCandidate&) = default;
PeerCandidate::PeerCandidate(PeerCandidate&&) = default;
PeerCandidate& PeerCandidate::operator=(PeerCandidate&&) = default;
PeerCandidate::~PeerCandidate() = default;

}  // namespace brave_transfer
