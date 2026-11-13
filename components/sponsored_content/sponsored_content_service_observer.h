/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SPONSORED_CONTENT_SPONSORED_CONTENT_SERVICE_OBSERVER_H_
#define BRAVE_COMPONENTS_SPONSORED_CONTENT_SPONSORED_CONTENT_SERVICE_OBSERVER_H_

#include <optional>

#include "base/observer_list_types.h"

namespace base {
class DictValue;
class FilePath;
}  // namespace base

namespace sponsored_content {

// A `dict` of `std::nullopt` means the file is missing or unreadable, so
// the observer should treat it as having no content, not keep stale content.
class SponsoredContentServiceObserver : public base::CheckedObserver {
 public:
  // Called when `campaigns.json` updates.
  virtual void OnSponsoredNewTabTakeoverContentDidUpdate(
      const base::FilePath& installed_dir,
      const std::optional<base::DictValue>& dict) {}

  // Called when `tiles.json` updates.
  virtual void OnSponsoredSiteContentDidUpdate(
      const base::FilePath& installed_dir,
      const std::optional<base::DictValue>& dict) {}

  // Called when `conversion-config.json` updates.
  virtual void OnSponsoredConversionConfigContentDidUpdate(
      const base::FilePath& installed_dir,
      const std::optional<base::DictValue>& dict) {}

  // Called when the component is unregistered.
  virtual void OnSponsoredContentComponentDidUnregister() {}

 protected:
  ~SponsoredContentServiceObserver() override = default;
};

}  // namespace sponsored_content

#endif  // BRAVE_COMPONENTS_SPONSORED_CONTENT_SPONSORED_CONTENT_SERVICE_OBSERVER_H_
