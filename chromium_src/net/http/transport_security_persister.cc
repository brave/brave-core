/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "net/http/transport_security_persister.h"

// Use upstream version of TransportSerurityState to reference
// TransportSecurityState::Delegate without build issues.
#define TransportSecurityState TransportSecurityState_ChromiumImpl
#include "src/net/http/transport_security_persister.cc"
#undef TransportSecurityState
