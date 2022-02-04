/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>

#ifndef BRAVE_IOS_BROWSER_API_PASSWORD_PASSWORD_STORE_LISTENER_IOS_H_
#define BRAVE_IOS_BROWSER_API_PASSWORD_PASSWORD_STORE_LISTENER_IOS_H_

#include "base/memory/ref_counted.h"
#include "brave/ios/browser/api/password/brave_password_observer.h"

#include "components/password_manager/core/browser/password_form.h"
#include "components/password_manager/core/browser/password_store.h"
#include "components/password_manager/core/browser/password_store_interface.h"

@interface PasswordStoreListenerImpl : NSObject <PasswordStoreListener>
- (instancetype)init:(id<PasswordStoreObserver>)observer
      passwordStore:(void*)store;
@end

namespace brave {
namespace ios {

class PasswordStoreListenerIOS : public password_manager::PasswordStoreInterface::Observer {
 public:
  explicit PasswordStoreListenerIOS(id<PasswordStoreObserver> observer,
      scoped_refptr<password_manager::PasswordStoreInterface> store);
  ~PasswordStoreListenerIOS() override;

 private:
  // Called when the contents of the password store change.
  void OnLoginsChanged(
      password_manager::PasswordStoreInterface* store,
      const password_manager::PasswordStoreChangeList& changes) override;
  void OnLoginsRetained(password_manager::PasswordStoreInterface* store,
                        const std::vector<password_manager::PasswordForm>&
                            retained_passwords) override;

  id<PasswordStoreObserver> observer_;
  scoped_refptr<password_manager::PasswordStoreInterface> store_;  // NOT OWNED
};

}  // namespace ios
}  // namespace brave

#endif  // BRAVE_IOS_BROWSER_API_PASSWORD_PASSWORD_STORE_LISTENER_IOS_H_

