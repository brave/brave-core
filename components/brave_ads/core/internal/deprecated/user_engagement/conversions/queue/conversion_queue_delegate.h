/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DEPRECATED_USER_ENGAGEMENT_CONVERSIONS_QUEUE_CONVERSION_QUEUE_DELEGATE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DEPRECATED_USER_ENGAGEMENT_CONVERSIONS_QUEUE_CONVERSION_QUEUE_DELEGATE_H_

#include "base/time/time.h"

namespace brave_ads {

struct ConversionInfo;

class ConversionQueueDelegate {
 public:
  // Invoked to tell the delegate that we added a conversion to the queue.
  virtual void OnDidAddConversionToQueue(const ConversionInfo& conversion) {}

  // Invoked to tell the delegate that we failed to add a conversion to the
  // queue.
  virtual void OnFailedToAddConversionToQueue(
      const ConversionInfo& conversion) {}

  // Invoked to tell the delegate that we will process the conversion queue.
  virtual void OnWillProcessConversionQueue(const ConversionInfo& conversion,
                                            base::Time process_at) {}

  // Invoked to tell the delegate that we processed the conversion queue.
  virtual void OnDidProcessConversionQueue(const ConversionInfo& conversion) {}

  // Invoked to tell the delegate that we failed to process the conversion
  // queue.
  virtual void OnFailedToProcessConversionQueue(
      const ConversionInfo& conversion) {}

  // Invoked to tell the delegate that we failed to process the next conversion.
  virtual void OnFailedToProcessNextConversionInQueue() {}

  // Invoked to tell the delegate that the conversion queue has been exhausted.
  virtual void OnDidExhaustConversionQueue() {}

 protected:
  virtual ~ConversionQueueDelegate() = default;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DEPRECATED_USER_ENGAGEMENT_CONVERSIONS_QUEUE_CONVERSION_QUEUE_DELEGATE_H_
