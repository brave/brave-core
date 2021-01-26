// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_P3A_BRAVE_P3A_COLLECTOR_H_
#define BRAVE_COMPONENTS_P3A_BRAVE_P3A_COLLECTOR_H_

namespace brave {

// Observer-type base class for implementors to announce to p3a service that
// they would like to send metrics that are accurate at the point of data
// sending. Instances should be added to a BraveP3AService instance via
// `BraveP3AService::AddCollector`.
class BraveP3ACollector {
  public:
    virtual void CollectMetrics() = 0;
};

}

#endif  // BRAVE_COMPONENTS_P3A_BRAVE_P3A_COLLECTOR_H_
