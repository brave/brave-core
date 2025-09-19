/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/update_client/privacy_preserving_protocol_handler.h"

#define ProtocolHandlerFactoryJSON PrivacyPreservingProtocolHandlerFactory
#include <chrome/updater/ping_configurator.cc>
#undef ProtocolHandlerFactoryJSON
