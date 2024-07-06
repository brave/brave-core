/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/pkcs11_api.h"

#include <memory>
#include <string>
#include <iostream>
#include "base/logging.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "brave/common/extensions/api/pkcs11.h"
#include "brave/third_party/botan/src/pkcs.h"

namespace extensions {
namespace api {

ExtensionFunction::ResponseAction Pkcs11GetSignatureFunction::Run() {
  absl::optional<pkcs11::GetSignature::Params> params =
  pkcs11::GetSignature::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  std::string module_path= std::string(params->module_path);
  std::string pin= std::string(params->pin);
  std::string hex= std::string(params->md_hash);

  char* module_path_ptr = const_cast<char*>(module_path.c_str());
  char* hex_ptr = const_cast<char*>(hex.c_str());
  char* pin_ptr = const_cast<char*>(pin.c_str());
  
  std::cout<<botan_high_level::pkcs11::sign_data(module_path_ptr, pin_ptr, hex_ptr)<<std::endl; 
  return RespondNow(WithArguments(botan_high_level::pkcs11::sign_data(module_path_ptr, pin_ptr, hex_ptr)));
}

}  // namespace api
}  // namespace extensions