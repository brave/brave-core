/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/bind.h"
#include "chrome/browser/password_manager/profile_password_store_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/password_manager/core/browser/password_form_manager_for_ui.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "url/gurl.h"

class PasswordManagerTest : public InProcessBrowserTest {
 public:
  PasswordManagerTest() = default;
  ~PasswordManagerTest() override = default;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    store_ = ProfilePasswordStoreFactory::GetForProfile(
        browser()->profile(), ServiceAccessType::EXPLICIT_ACCESS);
  }

 protected:
  scoped_refptr<password_manager::PasswordStoreInterface> store_;
};

IN_PROC_BROWSER_TEST_F(PasswordManagerTest,
                       SavePasswordAndOpenSettingsNoErrors) {
  // Create test credentials.
  password_manager::PasswordForm form;
  form.url = GURL("https://example.com");
  form.signon_realm = "https://example.com";
  form.username_value = u"test_user";
  form.password_value = u"test_password";
  form.scheme = password_manager::PasswordForm::Scheme::kHtml;
  base::RunLoop run_loop;
  store_->AddLogin(form, run_loop.QuitClosure());
  run_loop.Run();

  // Open password settings and expect no errors.
  content::WebContents* contents =
      chrome_test_utils::GetActiveWebContents(this);
  content::WebContentsConsoleObserver console_observer(contents);
  console_observer.SetFilter(base::BindLambdaForTesting(
      [](const content::WebContentsConsoleObserver::Message& message) {
        return message.log_level == blink::mojom::ConsoleMessageLevel::kError;
      }));
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), GURL(chrome::kChromeUIPasswordManagerURL)));

  // Wait for the password list to be populated.
  EXPECT_TRUE(content::ExecJs(contents, R"(
    (async () => {
      new Promise((resolve) => {
        function queryShadowRoot(node, selector) {
          const nodes = [...node.querySelectorAll(selector)];
          const nodeIterator = document.createNodeIterator(
            node,
            NodeFilter.SHOW_ELEMENT,
            (node) =>
              node instanceof Element && node.shadowRoot
                ? NodeFilter.FILTER_ACCEPT
                : NodeFilter.FILTER_REJECT
          );

          for (
            let currentNode = nodeIterator.nextNode();
            currentNode;
            currentNode = nodeIterator.nextNode()
          ) {
            nodes.push(...queryShadowRoot(currentNode.shadowRoot, selector));
          }

          return nodes;
        }

        function checkPasswords() {
          const password_items = queryShadowRoot(document, "password-list-item");
          if (password_items.length > 0) {
            resolve(true);
            return;
          }
          setTimeout(checkPasswords, 100);
        }
        checkPasswords();
      });
    })();
  )"));

  EXPECT_TRUE(console_observer.messages().empty());
}
