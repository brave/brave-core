/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_UTIL_H_
#define BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_UTIL_H_

#include "base/no_destructor.h"
#include "third_party/re2/src/re2/re2.h"

class GURL;

namespace speedreader {

// Helper class for testing URLs against precompiled regexes. This is a
// singleton so the cached regexes are created only once.
class URLReadableHintExtractor {
 public:
  static URLReadableHintExtractor* GetInstance() {
    static base::NoDestructor<URLReadableHintExtractor> instance;
    return instance.get();
  }

  URLReadableHintExtractor(const URLReadableHintExtractor&) = delete;
  URLReadableHintExtractor& operator=(const URLReadableHintExtractor&) = delete;
  ~URLReadableHintExtractor() = delete;

  bool HasHints(const GURL& url);

 private:
  // friend itself so GetInstance() can call private constructor.
  friend class base::NoDestructor<URLReadableHintExtractor>;
  URLReadableHintExtractor();

  const re2::RE2 path_single_component_hints_;
  const re2::RE2 path_multi_component_hints_;
};

}  // namespace speedreader

#endif  // BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_UTIL_H_
