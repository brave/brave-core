/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_LEGACY_STATE_WRITER_H_
#define BRAVELEDGER_LEGACY_STATE_WRITER_H_

#include <string>

namespace ledger {
namespace state {

template <class T, class U>
class Writer {
 public:
  virtual bool ToJson(
      T writer,
      const U&) const = 0;

  virtual std::string ToJson(
    const U&) const = 0;
};

}  // namespace state
}  // namespace ledger

#endif  // BRAVELEDGER_LEGACY_STATE_WRITER_H_
