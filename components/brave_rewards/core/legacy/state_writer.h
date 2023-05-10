/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LEGACY_STATE_WRITER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LEGACY_STATE_WRITER_H_

#include <string>

namespace brave_rewards::internal {
namespace state {

template <class T, class U>
class Writer {
 public:
  virtual bool ToJson(T writer, const U&) const = 0;

  virtual std::string ToJson(const U&) const = 0;
};

}  // namespace state
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LEGACY_STATE_WRITER_H_
