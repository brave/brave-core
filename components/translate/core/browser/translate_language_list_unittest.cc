#include "components/translate/core/browser/translate_language_list.h"

#include <string>
#include <vector>

#include "base/run_loop.h"
#include "base/stl_util.h"
#include "base/test/bind_test_util.h"
#include "base/test/scoped_command_line.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/translate/core/browser/translate_download_manager.h"
#include "components/translate/core/browser/translate_url_util.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace translate {

// Check that we don't send any network data to the translate server,
// even if translation is allowed and resource requests are allowed
TEST(TranslateLanguageListTest, GetSupportedLanguagesNoFetch) {
  base::test::ScopedTaskEnvironment scoped_task_environment;
  network::TestURLLoaderFactory test_url_loader_factory;
  scoped_refptr<network::SharedURLLoaderFactory> test_shared_loader_factory =
      base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
          &test_url_loader_factory);
  TranslateDownloadManager::GetInstance()->set_application_locale("en");
  TranslateDownloadManager::GetInstance()->set_url_loader_factory(
      test_shared_loader_factory);

  bool network_access_occurred = false;
  base::RunLoop loop;
  // Since translate is allowed by policy, we will schedule a language list
  // load. Intercept to ensure the URL is correct.
  test_url_loader_factory.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
                                     network_access_occurred = true;
                                     loop.Quit();
                                 }));

  // Populate supported languages.
  std::vector<std::string> languages;
  TranslateLanguageList language_list;
  language_list.SetResourceRequestsAllowed(true);
  language_list.GetSupportedLanguages(true /* translate_allowed */,
                                      &languages);

  // We should *not* have scheduled a language list load.
  EXPECT_FALSE(language_list.HasOngoingLanguageListLoadingForTesting());
  EXPECT_TRUE(test_url_loader_factory.pending_requests()->empty());
  EXPECT_FALSE(network_access_occurred);

  TranslateDownloadManager::GetInstance()->ResetForTesting();
}

}  // namespace translate
