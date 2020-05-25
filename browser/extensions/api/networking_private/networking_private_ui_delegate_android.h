/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef CHROME_BROWSER_EXTENSIONS_API_NETWORKING_PRIVATE_NETWORKING_PRIVATE_UI_DELEGATE_ANDROID_H_
#define CHROME_BROWSER_EXTENSIONS_API_NETWORKING_PRIVATE_NETWORKING_PRIVATE_UI_DELEGATE_ANDROID_H_

#include "base/macros.h"
#include "extensions/browser/api/networking_private/networking_private_delegate.h"

namespace android {
namespace extensions {

// Chrome OS implementation of NetworkingPrivateDelegate::UIDelegate.
class NetworkingPrivateUIDelegateAndroid
    : public ::extensions::NetworkingPrivateDelegate::UIDelegate {
 public:
  NetworkingPrivateUIDelegateAndroid();
  ~NetworkingPrivateUIDelegateAndroid() override;

  // NetworkingPrivateDelegate::UIDelegate
  void ShowAccountDetails(const std::string& guid) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(NetworkingPrivateUIDelegateAndroid);
};

}  // namespace android
}  // namespace chromeos

#endif  // CHROME_BROWSER_EXTENSIONS_API_NETWORKING_PRIVATE_NETWORKING_PRIVATE_UI_DELEGATE_ANDROID_H_
