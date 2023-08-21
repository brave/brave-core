/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_QUEUE_CONFIRMATION_QUEUE_DELEGATE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_QUEUE_CONFIRMATION_QUEUE_DELEGATE_H_

#include "base/time/time.h"

namespace brave_ads {

struct ConfirmationInfo;

class ConfirmationQueueDelegate {
 public:
  // Invoked to tell the delegate when we add a confirmation to the queue.
  virtual void OnDidAddConfirmationToQueue(
      const ConfirmationInfo& confirmation) {}

  // Invoked to tell the delegate when we will process the confirmation queue.
  virtual void OnWillProcessConfirmationQueue(
      const ConfirmationInfo& confirmation,
      base::Time process_at) {}

  // Invoked to tell the delegate when we process the confirmation queue.
  virtual void OnDidProcessConfirmationQueue(
      const ConfirmationInfo& confirmation) {}

  // Invoked to tell the delegate when we fail to process the confirmation
  // queue.
  virtual void OnFailedToProcessConfirmationQueue(
      const ConfirmationInfo& confirmation) {}

  // Invoked to tell the delegate when the confirmation queue has been
  // exhausted.
  virtual void OnDidExhaustConfirmationQueue() {}

 protected:
  virtual ~ConfirmationQueueDelegate() = default;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_QUEUE_CONFIRMATION_QUEUE_DELEGATE_H_
