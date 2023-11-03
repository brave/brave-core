// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_JSON_JSON_CONVERTER_H_
#define BRAVE_COMPONENTS_JSON_JSON_CONVERTER_H_

#include "base/component_export.h"
#include "base/no_destructor.h"
#include "brave/components/json/json_converter_impl.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace brave {
namespace json {

class COMPONENT_EXPORT(JSON_CONVERTER) JsonConverter {
 public:
  static JsonConverter& GetJsonConverter();

  JsonConverter(const JsonConverter&) = delete;
  JsonConverter& operator=(const JsonConverter&) = delete;

  ~JsonConverter();

  void ConvertAllNumbersToString(
      const std::string& json,
      const std::string& path,
      mojom::JsonConverter::ConvertAllNumbersToStringCallback callback);

 private:
  friend class base::NoDestructor<JsonConverter>;
  JsonConverter();
  void BindRemote();

  mojo::Remote<mojom::JsonConverter> converter_;
};
}  // namespace json
}  // namespace brave

#endif  // BRAVE_COMPONENTS_JSON_JSON_CONVERTER_H_
