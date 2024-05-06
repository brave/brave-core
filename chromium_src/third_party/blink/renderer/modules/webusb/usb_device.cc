/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/modules/webusb/usb_device.h"

#include "brave/third_party/blink/renderer/brave_farbling_constants.h"
#include "brave/third_party/blink/renderer/core/farbling/brave_session_cache.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"

#include "src/third_party/blink/renderer/modules/webusb/usb_device.cc"

namespace blink {

String USBDevice::serialNumber() const {
  String realSerialNumber = serialNumber_ChromiumImpl();
  if (realSerialNumber.length() > 0) {
    if (ExecutionContext* context = GetExecutionContext()) {
      if (brave::GetBraveFarblingLevelFor(
              context,
              ContentSettingsType::BRAVE_WEBCOMPAT_USB_DEVICE_SERIAL_NUMBER,
              BraveFarblingLevel::BALANCED) != BraveFarblingLevel::OFF) {
        WTF::StringBuilder result;
        result.Append(realSerialNumber);
        result.Append("WEBUSB_SERIAL_NUMBER");
        return brave::BraveSessionCache::From(*context).GenerateRandomString(
            result.ToString().Utf8(), 16);
      }
    }
  }
  return realSerialNumber;
}

}  // namespace blink
