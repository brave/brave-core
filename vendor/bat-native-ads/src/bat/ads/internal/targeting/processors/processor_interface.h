/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TARGETING_PROCESSORS_PROCESSOR_INTERFACE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TARGETING_PROCESSORS_PROCESSOR_INTERFACE_H_

namespace ads {
namespace targeting {
namespace processor {

template <typename T>
class ProcessorInterface {
 public:
  virtual ~ProcessorInterface() = default;

  virtual void Process(const T&) = 0;
};

}  // namespace processor
}  // namespace targeting
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TARGETING_PROCESSORS_PROCESSOR_INTERFACE_H_
