/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BRAVE_WALLET_JSON_CONVERTER_IMPL_H_
#define BRAVE_COMPONENTS_SERVICES_BRAVE_WALLET_JSON_CONVERTER_IMPL_H_

#include <string>
#include <vector>

#include "base/component_export.h"
#include "base/functional/callback_forward.h"
#include "brave/components/services/brave_wallet/public/mojom/json_converter.mojom.h"
#include "brave/components/services/brave_wallet/public/mojom/third_party_service.mojom.h"

namespace brave_wallet {

class COMPONENT_EXPORT(BRAVE_WALLET_SERVICE) JsonConverterImpl
    : public third_party_service::mojom::JsonConverter {
 public:
  JsonConverterImpl();

  JsonConverterImpl(const JsonConverterImpl&) = delete;
  JsonConverterImpl& operator=(const JsonConverterImpl&) = delete;

  ~JsonConverterImpl() override;

  // mojom::JsonConverter implementation:
  using JsonConverterStringCallback = base::OnceCallback<void(const absl::optional<std::string>&)>;
  void ConvertUint64ValueToString(
      const std::string& path, const std::string& json, bool optional, JsonConverterStringCallback callback) override;
  void ConvertInt64ValueToString(
      const std::string& path, const std::string& json, bool optional, JsonConverterStringCallback callback) override;
  void ConvertStringValueToUint64(
      const std::string& path, const std::string& json, bool optional, JsonConverterStringCallback callback) override;
  void ConvertStringValueToInt64(
      const std::string& path, const std::string& json, bool optional, JsonConverterStringCallback callback) override;
  void ConvertUint64InObjectArrayToString(
      const std::string& path_to_list, const std::string& path_to_object,
      const std::string& key, const std::string& json, JsonConverterStringCallback callback) override;
  void ConvertAllNumbersToString(
      const std::string& json, const std::string& path, JsonConverterStringCallback callback) override;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_SERVICES_BRAVE_WALLET_JSON_CONVERTER_IMPL_H_
