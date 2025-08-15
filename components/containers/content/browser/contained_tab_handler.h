// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_CONTAINERS_CONTENT_BROWSER_CONTAINED_TAB_HANDLER_H_
#define BRAVE_COMPONENTS_CONTAINERS_CONTENT_BROWSER_CONTAINED_TAB_HANDLER_H_

#include <string>

#include "base/component_export.h"

namespace containers {

class COMPONENT_EXPORT(CONTAINERS_CONTENT_BROWSER) ContainedTabHandler {
 public:
  virtual ~ContainedTabHandler() = default;

  // The prefix for the handler id.
  static constexpr char kIdPrefix[] = "containers-";

  // The id of the contained tab handler.
  virtual const std::string& GetId() const = 0;
};

}  // namespace containers

#endif  // BRAVE_COMPONENTS_CONTAINERS_CONTENT_BROWSER_CONTAINED_TAB_HANDLER_H_
