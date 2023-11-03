// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_JSON_JSON_CONVERTER_IMPL_H_
#define BRAVE_COMPONENTS_JSON_JSON_CONVERTER_IMPL_H_

#include "brave/components/json/json_converter.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace brave {
namespace json {

class JsonConverterImpl : public mojom::JsonConverter {
 public:
  explicit JsonConverterImpl(
      mojo::PendingReceiver<mojom::JsonConverter> receiver);

  JsonConverterImpl(const JsonConverterImpl&) = delete;
  JsonConverterImpl& operator=(const JsonConverterImpl&) = delete;

  ~JsonConverterImpl() override;

 private:
  // mojom::JsonConverter implementation.
  void ConvertAllNumbersToString(
      const std::string& json,
      const std::string& path,
      ConvertAllNumbersToStringCallback callback) override;

  mojo::Receiver<mojom::JsonConverter> receiver_;
};

}  // namespace json
}  // namespace brave

#endif  // BRAVE_COMPONENTS_JSON_JSON_CONVERTER_IMPL_H_
