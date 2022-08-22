/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_URL_SANITIZER_URL_SANITIZER_H_
#define BRAVE_COMPONENTS_URL_SANITIZER_URL_SANITIZER_H_

#include <string>

namespace brave {

class URLSanitizer {
 public:
  URLSanitizer() = default;
  ~URLSanitizer() = default;
  
  
  // Remove tracking query parameters from a GURL, leaving all
  // other parts untouched.
  static std::string StripQueryParameter(const std::string& query,
                                  const std::string& spec);
  
};

}  // namesapce brave

#endif // BRAVE_COMPONENTS_URL_SANITIZER_URL_SANITIZER_H_
