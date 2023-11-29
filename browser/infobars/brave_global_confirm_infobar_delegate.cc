/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/infobars/brave_global_confirm_infobar_delegate.h"

BraveGlobalConfirmInfobarDelegate::BraveGlobalConfirmInfobarDelegate() =
    default;
BraveGlobalConfirmInfobarDelegate::~BraveGlobalConfirmInfobarDelegate() =
    default;

void BraveGlobalConfirmInfobarDelegate::AddObserver(Observer* obs) {
  observer_list_.AddObserver(obs);
}

void BraveGlobalConfirmInfobarDelegate::RemoveObserver(Observer* obs) {
  observer_list_.RemoveObserver(obs);
}

bool BraveGlobalConfirmInfobarDelegate::Accept() {
  for (Observer& observer : observer_list_) {
    observer.OnInfoBarClosed();
  }

  return true;
}

bool BraveGlobalConfirmInfobarDelegate::Cancel() {
  for (Observer& observer : observer_list_) {
    observer.OnInfoBarClosed();
  }

  return true;
}

void BraveGlobalConfirmInfobarDelegate::InfoBarDismissed() {
  for (Observer& observer : observer_list_) {
    observer.OnInfoBarClosed();
  }
}
