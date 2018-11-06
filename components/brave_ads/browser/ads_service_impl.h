/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_ADS_SERVICE_IMPL_
#define BRAVE_COMPONENTS_BRAVE_ADS_ADS_SERVICE_IMPL_

#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "bat/ads/ads_client.h"
#include "brave/components/brave_ads/browser/ads_service.h"

class Profile;

namespace base {
class SequencedTaskRunner;
}

namespace ads {
class Ads;
}

namespace brave_ads {

class AdsServiceImpl : public AdsService,
                       public ads::AdsClient,
                       public base::SupportsWeakPtr<AdsServiceImpl> {
 public:
  explicit AdsServiceImpl(Profile* profile);
  ~AdsServiceImpl() override;

  bool is_enabled() override;

 private:
  void Init();

  // AdsClient implementation
  const ads::ClientInfo GetClientInfo() const override;
  std::string SetLocale(const std::string& locale) override;
  const std::vector<std::string>& GetLocales() const override;
  const std::string GenerateUUID() const override;
  const std::string GetSSID() const override;
  void ShowAd(const std::unique_ptr<ads::AdInfo> info) override {}
  void SetTimer(const uint64_t time_offset, uint32_t& timer_id) override {}
  void KillTimer(uint32_t& timer_id) override {};
  std::unique_ptr<ads::URLSession> URLSessionTask(
      const std::string& url,
      const std::vector<std::string>& headers,
      const std::string& content,
      const std::string& content_type,
      const ads::URLSession::Method& method,
      ads::URLSessionCallbackHandlerCallback callback) override;
  void Save(const std::string& name,
            const std::string& value,
            ads::OnSaveCallback callback) override;
  void Load(const std::string& name,
            ads::OnLoadCallback callback) override;
  void Reset(const std::string& name,
             ads::OnResetCallback callback) override;
  void GetAds(
      const std::string& winning_category,
      ads::CallbackHandler* callback_handler) override {}
  void GetSampleCategory(ads::CallbackHandler* callback_handler) override {}
  bool GetUrlComponents(
      const std::string& url,
      ads::UrlComponents* components) const override;
  void EventLog(const std::string& json) override {}
  std::ostream& Log(const char* file,
                    int line,
                    const ads::LogLevel log_level) const override;

  void OnLoaded(const ads::OnLoadCallback& callback,
                const std::string& value);

  Profile* profile_;  // NOT OWNED
  const scoped_refptr<base::SequencedTaskRunner> file_task_runner_;
  const base::FilePath base_path_;

  std::unique_ptr<ads::Ads> ads_;

  DISALLOW_COPY_AND_ASSIGN(AdsServiceImpl);
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_ADS_SERVICE_IMPL_
