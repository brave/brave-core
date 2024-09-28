/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/webcompat_reporter/browser/webcompat_reporter_service.h"

#include <memory>

#include "base/test/mock_callback.h"
#include "base/version.h"
#include "brave/components/webcompat_reporter/browser/webcompat_report_uploader.h"
#include "components/component_updater/component_updater_service.h"
#include "components/update_client/update_client.h"
#include "content/public/test/browser_task_environment.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

using testing::_;

namespace component_updater {
class MockComponentUpdateService : public ComponentUpdateService {
public:
  MOCK_METHOD(void,
              AddObserver,
              (update_client::UpdateClient::Observer*),
              (override));
  MOCK_METHOD(void,
              RemoveObserver,
              (update_client::UpdateClient::Observer*),
              (override));
  MOCK_METHOD(base::Version,
              GetRegisteredVersion,
              (const std::string&),
              (override));
  MOCK_METHOD(std::vector<ComponentInfo>, GetComponents, (), (const, override));
  MOCK_METHOD(base::Version,
              GetMaxPreviousProductVersion,
              (const std::string& app_id),
              (override));
  MOCK_METHOD(bool,
              RegisterComponent,
              (const ComponentRegistration& component),
              (override));
  MOCK_METHOD(bool, UnregisterComponent, (const std::string& id), (override));
  MOCK_METHOD(std::vector<std::string>, GetComponentIDs, (), (const, override));
  MOCK_METHOD(OnDemandUpdater&, GetOnDemandUpdater, (), (override));
  MOCK_METHOD(void,
              MaybeThrottle,
              (const std::string& id, base::OnceClosure callback),
              (override));
  MOCK_METHOD(bool,
              GetComponentDetails,
              (const std::string& id, update_client::CrxUpdateItem* item),
              (const, override));
};
}  // namespace component_updater

namespace webcompat_reporter {

class MockWebcompatReportUploader : public WebcompatReportUploader {
 public:
  MockWebcompatReportUploader() : WebcompatReportUploader(nullptr) {}

  ~MockWebcompatReportUploader() override {}
  MOCK_METHOD(void, SubmitReport, (const Report& report), (override));
};

class WebcompatReporterServiceUnitTest : public testing::Test {
 public:
  WebcompatReporterServiceUnitTest()
      : webcompat_reporter_service_(std::unique_ptr<WebcompatReporterService>(
            new WebcompatReporterService())),
        updater_(std::unique_ptr<component_updater::MockComponentUpdateService>(
            new component_updater::MockComponentUpdateService())) {
    webcompat_reporter_service_->SetUpWebcompatReporterServiceForTest(
        std::unique_ptr<WebcompatReportUploader>(
            new MockWebcompatReportUploader()),
        updater_.get());
  }

  ~WebcompatReporterServiceUnitTest() override = default;

  MockWebcompatReportUploader* GetMockWebcompatReportUploader() {
    return static_cast<MockWebcompatReportUploader*>(
        webcompat_reporter_service_->report_uploader_.get());
  }

 protected:
  std::unique_ptr<WebcompatReporterService> webcompat_reporter_service_;
  std::unique_ptr<component_updater::ComponentUpdateService> updater_;

 private:
  content::BrowserTaskEnvironment task_environment_;
};

TEST_F(WebcompatReporterServiceUnitTest, SubmitReport) {
  Report report;
  webcompat_reporter_service_->SubmitWebcompatReport(report);
  EXPECT_CALL(*GetMockWebcompatReportUploader(), SubmitReport(_)).Times(1);
  EXPECT_CALL(*static_cast<component_updater::MockComponentUpdateService*>(
                  updater_.get()),
              GetComponents)
      .Times(0);
}

TEST_F(WebcompatReporterServiceUnitTest, SubmitReportMojo) {
  base::MockCallback<base::OnceCallback<void()>> callback;
  auto report_info = mojom::ReportInfo::New();
  std::vector<component_updater::ComponentInfo> get_components_ret_value{
      component_updater::ComponentInfo("id", "fp", u"name",
                                       base::Version("1.234"), "c")};
  EXPECT_CALL(*static_cast<component_updater::MockComponentUpdateService*>(updater_.get()), GetComponents)
      .Times(1)
      .WillOnce(testing::Return(get_components_ret_value));
  EXPECT_CALL(*GetMockWebcompatReportUploader(), SubmitReport(_)).Times(1);
  EXPECT_CALL(callback, Run()).Times(1);

  webcompat_reporter_service_->SubmitWebcompatReport(std::move(report_info),
                                                     callback.Get());
}

}  // namespace webcompat_reporter
