/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "rewards_service_browsertest_utils.h"

#include "content/public/test/browser_test_utils.h"

namespace rewards_service_browsertest_utils {

static const std::string kWaitForElementToAppearScript = R"(
    const waitForElementToAppear = (selector) => {
      const TIMEOUT_SECONDS = 5;

      return new Promise((resolve, reject) => {
        let element = document.querySelector(selector);
        if (element) {
          resolve(element);
          return;
        }

        const timerID = window.setTimeout(() => {
          observer.disconnect();
          reject(new Error("Timed out waiting for '" + selector + "'."));
        }, TIMEOUT_SECONDS * 1000);

        const observer = new MutationObserver(({}, observer) => {
          let element = document.querySelector(selector);
          if (element) {
            clearTimeout(timerID);
            observer.disconnect();
            resolve(element);
          }
        });
        observer.observe(document.documentElement,
            { childList: true, subtree: true });
      });
    };
)";

void WaitForElementToAppear(content::WebContents* context,
      const std::string& selector) {
  auto script = kWaitForElementToAppearScript +
      content::JsReplace(R"(
          new Promise(async (resolve, reject) => {
            try {
              const selector = $1;

              const element = await waitForElementToAppear(selector);
              console.log("XXXX", element);
              resolve(true);
            } catch (error) {
              reject(error);
            }
          })
      )",
      selector);

  auto result = EvalJs(
      context,
      script,
      content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
      content::ISOLATED_WORLD_ID_CONTENT_END);

  ASSERT_EQ(true, result);
}

void WaitForElementToEqual(content::WebContents* context,
      const std::string& selector, const std::string& expectedValue) {
  auto script = kWaitForElementToAppearScript +
      content::JsReplace(R"(
          new Promise(async (resolve, reject) => {
            const TIMEOUT_SECONDS = 5;
            const selector = $1;
            const expectedValue = $2;

            try {
              let element = await waitForElementToAppear(selector);

              if (element.innerText === expectedValue) {
                resolve(true);
                return;
              }

              const timerID = window.setTimeout(() => {
                observer.disconnect();
                reject(new Error("Timed out waiting for '" + selector + "' " +
                    "to equal '" + expectedValue + "'"));
              }, TIMEOUT_SECONDS * 1000);

              const observer = new MutationObserver(({}, observer) => {
                let element = document.querySelector(selector);
                if (!element) {
                  return;
                }

                if (element.innerText === expectedValue) {
                  clearTimeout(timerID);
                  observer.disconnect();
                  resolve(true);
                }
              });
              observer.observe(document.documentElement,
                { characterData: true, childList: true, subtree: true });
            } catch(error) {
              reject(error);
            }
        });
      )",
      selector,
      expectedValue);

  auto result = EvalJs(
      context,
      script,
      content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
      content::ISOLATED_WORLD_ID_CONTENT_END);

  ASSERT_EQ(true, result);
}

void WaitForElementToContain(content::WebContents* context,
      const std::string& selector, const std::string& substring) {
  auto script = kWaitForElementToAppearScript +
      content::JsReplace(R"(
          new Promise(async (resolve, reject) => {
            const TIMEOUT_SECONDS = 5;
            const selector = $1;
            const substring = $2;

            try {
              let element = await waitForElementToAppear(selector);

              if (element.innerText.indexOf(substring) !== -1) {
                resolve(true);
                return;
              }

              const timerID = window.setTimeout(() => {
                observer.disconnect();
                reject(new Error("Timed out waiting for '" + selector + "' " +
                    "to contain '" + substring + "'"));
              }, TIMEOUT_SECONDS * 1000);

              const observer = new MutationObserver(({}, observer) => {
                let element = document.querySelector(selector);
                if (!element) {
                  return;
                }

                if (element.innerText.indexOf(substring) !== -1) {
                  clearTimeout(timerID);
                  observer.disconnect();
                  resolve(true);
                }
              });
              observer.observe(document.documentElement,
                  { characterData: true, childList: true, subtree: true });
            } catch(error) {
              reject(error);
            }
          });
      )",
      selector,
      substring);

  auto result = EvalJs(
      context,
      script,
      content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
      content::ISOLATED_WORLD_ID_CONTENT_END);

  ASSERT_EQ(true, result);
}

void WaitForElementThenClick(content::WebContents* context,
    const std::string& selector) {
   auto script = kWaitForElementToAppearScript +
      content::JsReplace(R"(
          new Promise(async (resolve, reject) => {
            try {
              const selector = $1;

              const element = await waitForElementToAppear(selector);
              element.click();
              resolve(true);
            } catch(error) {
              reject(error);
            }
          })
      )",
      selector);

  auto result = EvalJs(
      context,
      script,
      content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
      content::ISOLATED_WORLD_ID_CONTENT_END);

  ASSERT_EQ(true, result);
}

} // namespace rewards_service_browsertest_utils
