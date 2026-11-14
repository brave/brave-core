// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/ai_chat_settings_helper.h"

#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/strings/strcat.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "brave/browser/ai_chat/model_service_factory.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/browser/model_validator.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/settings_helper.mojom.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/tab_list/tab_list_interface.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/os_crypt/async/browser/os_crypt_async.h"
#include "components/os_crypt/async/browser/test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"

namespace ai_chat {

class AIChatSettingsCryptoBrowserTest : public InProcessBrowserTest {
 public:
  AIChatSettingsCryptoBrowserTest() {
    features_.InitWithFeatures({features::kAIChat},
                               {features::kAIChatRemoteModelsConfig});
  }

  void SetUpBrowserContextKeyedServices(
      content::BrowserContext* context) override {
    ModelServiceFactory::GetInstance()->SetTestingFactory(
        context, base::BindRepeating(
                     &AIChatSettingsCryptoBrowserTest::CreateModelService,
                     base::Unretained(this)));
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    model_service_ = ModelServiceFactory::GetForBrowserContext(GetProfile());
    ASSERT_TRUE(model_service_);
  }

 protected:
  std::unique_ptr<KeyedService> CreateModelService(
      content::BrowserContext* context) {
    if (!os_crypt_async_) {
      os_crypt_async_ = os_crypt_async::GetTestOSCryptAsyncForTesting(true);
    }
    return std::make_unique<ModelService>(
        Profile::FromBrowserContext(context)->GetPrefs(), os_crypt_async_.get(),
        network::NetworkContextGetter(), nullptr, base::FilePath());
  }

  content::WebContents* web_contents() {
    return GetTabListInterface()->GetActiveTab()->GetContents();
  }

  mojom::ModelPtr MakeModel(const std::string& key) {
    auto model = mojom::Model::New();
    model->display_name = "Synthetic model";
    auto options = mojom::CustomModelOptions::New();
    options->model_request_name = "test-model";
    options->context_size = kDefaultCustomModelContextSize;
    options->endpoint = GURL("https://example.com/v1/chat/completions");
    options->api_key = key;
    model->options =
        mojom::ModelOptions::NewCustomModelOptions(std::move(options));
    return model;
  }

  const base::ListValue& StoredModels() {
    return GetProfile()->GetPrefs()->GetList("brave.ai_chat.custom_models");
  }

  void SetCryptoUnavailable() {
    model_service_->SetEncryptorForTesting(
        os_crypt_async::GetTestEncryptorWithoutKeysForTesting());
  }

  void RestoreCrypto() {
    base::test::TestFuture<scoped_refptr<os_crypt_async::Encryptor>> ready;
    os_crypt_async_->GetInstance(ready.GetCallback());
    model_service_->SetEncryptorForTesting(ready.Get());
  }

  void OpenForm(bool edit) {
    ASSERT_TRUE(ui_test_utils::NavigateToURL(
        browser(), GURL(edit ? "chrome://settings/leo-ai/add-model?index=0"
                             : "chrome://settings/leo-ai/add-model")));
    ASSERT_EQ(true, content::EvalJs(web_contents(), R"JS(
      (async () => {
        function roots() {
          const result = [document];
          for (let i = 0; i < result.length; i++) {
            for (const el of result[i].querySelectorAll('*')) {
              if (el.shadowRoot) result.push(el.shadowRoot);
            }
          }
          return result;
        }
        function find(selector) {
          for (const root of roots()) {
            const el = root.querySelector(selector);
            if (el) return el;
          }
          return null;
        }
        function wait(predicate) {
          return new Promise((resolve, reject) => {
            const observed = new Set();
            const observer = new MutationObserver(check);
            const timeout = setTimeout(() => {
              observer.disconnect();
              reject(new Error('Timed out waiting for model form state'));
            }, 20000);
            function check() {
              if (predicate()) {
                clearTimeout(timeout);
                observer.disconnect();
                resolve();
                return;
              }
              for (const root of roots()) {
                if (!observed.has(root)) {
                  observed.add(root);
                  observer.observe(root, {subtree: true, childList: true,
                                         attributes: true, characterData: true});
                }
              }
            }
            check();
          });
        }
        await wait(() => {
          const candidate = find('model-config-ui');
          return !!candidate && (!location.search.includes('index=') ||
            !!candidate.modelItem?.options.customModelOptions);
        });
        const form = find('model-config-ui');
        async function submit() {
          const page = form.getRootNode().host;
          const original = page.onModelConfigSave_;
          let completion;
          page.onModelConfigSave_ = function(event) {
            completion = original.call(this, event);
            return completion;
          };
          try {
            form.handleClick_();
            if (!completion) throw new Error('Form did not submit');
            await completion;
          } finally {
            page.onModelConfigSave_ = original;
          }
        }
        function errorVisible(messageKey) {
          const input = form.shadowRoot.querySelector('#apiKeyInput');
          const slot = input?.shadowRoot?.querySelector('slot[name="errors"]');
          const error = slot?.assignedElements({flatten: true})[0];
          return input?.showErrors === true && !!error &&
                 error.getBoundingClientRect().height > 0 &&
                 error.textContent.trim() === form.i18n(messageKey);
        }
        function apiKeyFocused() {
          const input = form.shadowRoot.querySelector('#apiKeyInput');
          return input?.shadowRoot?.activeElement?.matches('input') === true;
        }
        window.byomTest = {form, wait, submit, errorVisible, apiKeyFocused};
        return true;
      })()
    )JS"));
  }

  content::EvalJsResult Eval(std::string_view code) {
    return content::EvalJs(
        web_contents(),
        base::StrCat({"(async () => { const form = window.byomTest.form; ",
                      code, " })()"}));
  }

  raw_ptr<ModelService> model_service_ = nullptr;
  std::unique_ptr<os_crypt_async::OSCryptAsync> os_crypt_async_;
  base::test::ScopedFeatureList features_;
};

IN_PROC_BROWSER_TEST_F(AIChatSettingsCryptoBrowserTest,
                       HelperReportsEncryptionFailure) {
  SetCryptoUnavailable();
  AIChatSettingsHelper helper(GetProfile());
  base::test::TestFuture<mojom::OperationResult> result;
  helper.AddCustomModel(MakeModel("synthetic-key"), result.GetCallback());
  EXPECT_EQ(result.Get(), mojom::OperationResult::EncryptionFailed);
  EXPECT_TRUE(StoredModels().empty());
}

IN_PROC_BROWSER_TEST_F(AIChatSettingsCryptoBrowserTest,
                       AddFailurePreservesFormAndCanRetry) {
  SetCryptoUnavailable();
  OpenForm(false);
  EXPECT_EQ(true, Eval(R"JS(
    form.label = 'Synthetic model';
    form.modelRequestName = 'test-model';
    form.endpointUrl = 'https://example.com/v1/chat/completions';
    form.contextSize = 4000;
    form.onModelApiKeyChange_({value: 'synthetic-key'});
    await window.byomTest.submit();
    await window.byomTest.wait(() => window.byomTest.errorVisible(
      'braveLeoAssistantApiKeyEncryptionError'));
    return location.pathname.endsWith('/add-model') &&
           form.label === 'Synthetic model' && form.apiKey === 'synthetic-key' &&
           window.byomTest.apiKeyFocused();
  )JS"));
  EXPECT_TRUE(StoredModels().empty());

  RestoreCrypto();
  EXPECT_EQ(true, Eval(R"JS(
    await window.byomTest.submit();
    await window.byomTest.wait(() => location.pathname === '/leo-ai');
    return true;
  )JS"));
  auto models = model_service_->GetCustomModels();
  ASSERT_EQ(models.size(), 1u);
  EXPECT_EQ(models[0]->options->get_custom_model_options()->api_key,
            "synthetic-key");
}

IN_PROC_BROWSER_TEST_F(AIChatSettingsCryptoBrowserTest,
                       SaveFailurePreservesStoredKeyAndForm) {
  ASSERT_EQ(model_service_->AddCustomModel(MakeModel("synthetic-original-key")),
            mojom::OperationResult::Success);
  const auto before = StoredModels().Clone();
  OpenForm(true);
  SetCryptoUnavailable();
  EXPECT_EQ(true, Eval(R"JS(
    form.label = 'Edited model';
    form.onModelApiKeyChange_({value: 'synthetic-new-key'});
    await window.byomTest.submit();
    await window.byomTest.wait(() => window.byomTest.errorVisible(
      'braveLeoAssistantApiKeyEncryptionError'));
    return location.pathname.endsWith('/add-model') &&
           form.label === 'Edited model' && form.apiKey === 'synthetic-new-key' &&
           window.byomTest.apiKeyFocused();
  )JS"));
  EXPECT_EQ(StoredModels(), before);
}

IN_PROC_BROWSER_TEST_F(AIChatSettingsCryptoBrowserTest,
                       UnreadableKeyIsVisibleAndCannotBeClearedByEditing) {
  ASSERT_EQ(model_service_->AddCustomModel(MakeModel("synthetic-key")),
            mojom::OperationResult::Success);
  SetCryptoUnavailable();
  const auto before = StoredModels().Clone();
  OpenForm(true);
  EXPECT_EQ(true, Eval(R"JS(
    await window.byomTest.wait(() => window.byomTest.errorVisible(
      'braveLeoAssistantApiKeyDecryptionError'));
    if (!form.apiKeyErrorMessage || form.apiKey !== '') return false;
    form.label = 'Edited model';
    await window.byomTest.submit();
    await window.byomTest.wait(() => form.isApiKeyInvalid);
    return location.pathname.endsWith('/add-model') &&
           window.byomTest.errorVisible('braveLeoAssistantApiKeyDecryptionError') &&
           window.byomTest.apiKeyFocused();
  )JS"));
  EXPECT_EQ(StoredModels(), before);
}

IN_PROC_BROWSER_TEST_F(AIChatSettingsCryptoBrowserTest,
                       EmptyKeyModelDoesNotRequireEncryption) {
  SetCryptoUnavailable();
  OpenForm(false);
  EXPECT_EQ(true, Eval(R"JS(
    form.label = 'Local model';
    form.modelRequestName = 'local-model';
    form.endpointUrl = 'https://example.com/v1/chat/completions';
    form.contextSize = 4000;
    await window.byomTest.submit();
    await window.byomTest.wait(() => location.pathname === '/leo-ai');
    return true;
  )JS"));
  ASSERT_EQ(StoredModels().size(), 1u);
}

}  // namespace ai_chat
