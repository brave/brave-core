#include "chrome/browser/profile_resetter/reset_report_uploader.h"

#include "base/test/bind_test_util.h"
#include "base/test/task_environment.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

class ResetReportUploaderTest : public testing::Test {
 public:
  ResetReportUploaderTest()
      : test_shared_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &test_url_loader_factory_)) {}

 protected:
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory() {
    return test_shared_loader_factory_;
  }
  network::TestURLLoaderFactory* test_url_loader_factory() {
    return &test_url_loader_factory_;
  }

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  network::TestURLLoaderFactory test_url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> test_shared_loader_factory_;
};

TEST_F(ResetReportUploaderTest, NoFetch) {
  bool network_access_occurred = false;
  test_url_loader_factory()->SetInterceptor(
    base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
                                   network_access_occurred = true;
                               }));
  ResetReportUploader* uploader =
    new ResetReportUploader(shared_url_loader_factory());
  uploader->DispatchReportInternal("");
  EXPECT_FALSE(network_access_occurred);
}
