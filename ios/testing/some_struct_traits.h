// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_TESTING_SOME_STRUCT_TRAITS_H_
#define BRAVE_IOS_TESTING_SOME_STRUCT_TRAITS_H_

#include "brave/ios/testing/some_service.mojom-shared.h"
#include "brave/ios/testing/some_struct_cpp.h"
#include "mojo/public/cpp/bindings/struct_traits.h"

namespace mojo {

template <>
struct StructTraits<mojom::SomeStructDataView, SomeStructCpp> {
  static const std::string& value(const SomeStructCpp& in) { return in.value; }
  static int32_t count(const SomeStructCpp& in) { return in.count; }

  static bool Read(mojom::SomeStructDataView data, SomeStructCpp* out) {
    out->count = data.count();
    return data.ReadValue(&out->value);
  }
};

}  // namespace mojo

#endif  // BRAVE_IOS_TESTING_SOME_STRUCT_TRAITS_H_
