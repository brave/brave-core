// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/json/json_converter.h"

#include "base/time/time.h"
#include "content/public/browser/service_process_host.h"

namespace brave {
namespace json {

namespace {
constexpr base::TimeDelta kServiceProcessIdleTimeout{base::Seconds(5)};
}  // namespace

// static
JsonConverter& JsonConverter::GetJsonConverter() {
  static base::NoDestructor<JsonConverter> converter;
  return *converter;
}

JsonConverter::JsonConverter() = default;
JsonConverter::~JsonConverter() = default;

void JsonConverter::ConvertAllNumbersToString(
    const std::string& json,
    const std::string& path,
    mojom::JsonConverter::ConvertAllNumbersToStringCallback callback) {
  BindRemote();
  converter_->ConvertAllNumbersToString(json, path, std::move(callback));
}

void JsonConverter::BindRemote() {
  if (converter_.is_bound()) {
    return;
  }

  content::ServiceProcessHost::Launch(converter_.BindNewPipeAndPassReceiver(),
                                      content::ServiceProcessHost::Options()
                                          .WithDisplayName("Json Converter")
                                          .Pass());

  converter_.reset_on_disconnect();
  converter_.reset_on_idle_timeout(kServiceProcessIdleTimeout);
}

}  // namespace json
}  // namespace brave
