/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/translate/core/browser/translate_language_list.h"

#include <string>
#include <vector>

#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "brave/browser/translate/buildflags/buildflags.h"
#include "components/translate/core/browser/translate_download_manager.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace translate {

#if !BUILDFLAG(ENABLE_BRAVE_TRANSLATE_GO)
TEST(TranslateLanguageListTest, GetSupportedLanguagesNoFetch) {
  base::test::TaskEnvironment task_environment;
  network::TestURLLoaderFactory test_url_loader_factory;
  scoped_refptr<network::SharedURLLoaderFactory> test_shared_loader_factory =
      base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
          &test_url_loader_factory);
  TranslateDownloadManager::GetInstance()->set_application_locale("en");
  TranslateDownloadManager::GetInstance()->set_url_loader_factory(
      test_shared_loader_factory);

  // Since translate is allowed by policy, we will schedule a language list
  // load. Intercept to ensure the URL is correct.
  bool network_access_occurred = false;
  base::RunLoop loop;

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
#endif

}  // namespace translate
