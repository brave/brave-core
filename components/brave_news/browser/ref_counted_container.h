/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_REF_COUNTED_CONTAINER_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_REF_COUNTED_CONTAINER_H_

#include "base/memory/ref_counted.h"

namespace brave_news {

// A simple wrapper for RefCountedThreadSafe that holds a container like
// vector, flat_map, etc. The purpose of this class is almost same with the
// base::RefCountedData. But base::RefCountedData is not working when we want to
// have a const container that contains non-copyable types. Typically,
// Mojo struct ptr is the case.
//
// Example:
//
// This is not working, as template is not deduced correctly - deduced to
// copy constructor.
//
// ```
// scoped_ref_ptr<
//     base::RefCountedData<const std::vector<mojom::FeedItemMetadataPtr>>>
//     data = base::MakeRefCounted<
//         base::RefCountedData<const std::vector<mojom::FeedItemMetadataPtr>>>(
//             std::move(existing_data));
// ```
//
// So instead, we can use RefCountedContainer like below.
//
// ```
// scoped_ref_ptr<
//     RefCountedContainer<const std::vector<mojom::FeedItemMetadataPtr>>>
//     data = base::MakeRefCounted<
//         RefCountedContainer<const std::vector<mojom::FeedItemMetadataPtr>>>(
//             std::move(existing_data));
// ```
//
template <class ContainerType>
class RefCountedContainer
    : public base::RefCountedThreadSafe<RefCountedContainer<ContainerType>> {
 public:
  RefCountedContainer() = default;
  RefCountedContainer(std::remove_const_t<ContainerType>&& data)
      : data(std::move(data)) {}
  RefCountedContainer(RefCountedContainer&& other) {
    data = std::move(other.data);
  }

  ContainerType data;

 private:
  friend class base::RefCountedThreadSafe<RefCountedContainer<ContainerType>>;
};

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_REF_COUNTED_CONTAINER_H_
