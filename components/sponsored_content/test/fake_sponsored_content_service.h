/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SPONSORED_CONTENT_TEST_FAKE_SPONSORED_CONTENT_SERVICE_H_
#define BRAVE_COMPONENTS_SPONSORED_CONTENT_TEST_FAKE_SPONSORED_CONTENT_SERVICE_H_

#include <cstddef>
#include <optional>

#include "base/values.h"
#include "brave/components/sponsored_content/sponsored_content_service.h"

namespace base {
class FilePath;
}  // namespace base

namespace sponsored_content::test {

class FakeSponsoredContentService final : public SponsoredContentService {
 public:
  FakeSponsoredContentService();

  FakeSponsoredContentService(const FakeSponsoredContentService&) = delete;
  FakeSponsoredContentService& operator=(const FakeSponsoredContentService&) =
      delete;

  ~FakeSponsoredContentService() override;

  void RegisterSponsoredContentComponent() override;
  void UnregisterSponsoredContentComponent() override;

  void NotifySponsoredNewTabTakeoverContent(const base::FilePath& installed_dir,
                                            const base::DictValue& dict);
  void NotifySponsoredSiteContent(const base::FilePath& installed_dir,
                                  const base::DictValue& dict);
  void NotifySponsoredConversionConfigContent(
      const base::FilePath& installed_dir,
      const base::DictValue& dict);

  size_t register_call_count() const { return register_call_count_; }
  size_t unregister_call_count() const { return unregister_call_count_; }

 private:
  // Replays the last-notified content to observers, instead of reading real
  // files.
  void LoadAndNotifyContent(const base::FilePath& installed_dir) override;

  size_t register_call_count_ = 0;
  size_t unregister_call_count_ = 0;

  std::optional<base::DictValue> last_sponsored_new_tab_takeover_content_;
  std::optional<base::DictValue> last_sponsored_site_content_;
  std::optional<base::DictValue> last_sponsored_conversion_config_content_;
};

}  // namespace sponsored_content::test

#endif  // BRAVE_COMPONENTS_SPONSORED_CONTENT_TEST_FAKE_SPONSORED_CONTENT_SERVICE_H_
