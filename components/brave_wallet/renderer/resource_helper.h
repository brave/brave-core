/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_RENDERER_RESOURCE_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_RENDERER_RESOURCE_HELPER_H_

#include <optional>
#include <string>


namespace brave_wallet {

std::string LoadDataResource(const int id);
std::optional<std::string> LoadImageResourceAsDataUrl(const int id);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_RENDERER_RESOURCE_HELPER_H_
