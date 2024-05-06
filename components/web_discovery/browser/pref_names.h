/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_PREF_NAMES_H_
#define BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_PREF_NAMES_H_

namespace web_discovery {

inline constexpr char kCredentialRSAPrivateKey[] =
    "brave.web_discovery.rsa_priv_key";
inline constexpr char kCredentialRSAPublicKey[] =
    "brave.web_discovery.rsa_pub_key";
inline constexpr char kAnonymousCredentialsDict[] =
    "brave.web_discovery.anon_creds";

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_PREF_NAMES_H_
