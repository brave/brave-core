/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_context_util.h"

#include "base/strings/stringprintf.h"
#include "content/public/test/browser_test_utils.h"

namespace brave_rewards::test_util {

namespace {

constexpr char kWaitForElementToAppearScript[] = R"(
    const waitForElementToAppear = (selector) => {
      const TIMEOUT_SECONDS = 10;

      return new Promise((resolve, reject) => {
        let element = document.querySelector(selector);
        if (element) {
          resolve(element);
          return;
        }

        const timerID = window.setTimeout(() => {
          observer.disconnect();
          let element = document.querySelector(selector);
          if (element) {
            resolve(element);
          } else {
            reject(new Error("Timed out waiting for '" + selector + "'."));
          }
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

}  // namespace

void WaitForElementToAppear(
    content::WebContents* context,
    const std::string& selector,
    bool should_appear) {
  if (!context)
    return;

  auto script = kWaitForElementToAppearScript +
      content::JsReplace(R"(
          new Promise(async (resolve, reject) => {
            try {
              const selector = $1;

              const element = await waitForElementToAppear(selector);
              resolve(true);
            } catch (error) {
              resolve(false);
            }
          })
      )",
      selector);

  auto result = EvalJs(
      context,
      script,
      content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
      content::ISOLATED_WORLD_ID_CONTENT_END);

  ASSERT_EQ(should_appear, result);
}

void WaitForElementToEqual(
    content::WebContents* context,
    const std::string& selector,
    const std::string& expectedValue) {
  if (!context)
    return;

  std::string script = R"(
    new Promise(async (resolve, reject) => {
      const TIMEOUT_SECONDS = 5;
      const selector = $1;
      const expectedValue = $2;
      let currentValue = "";

      try {
        let element = await waitForElementToAppear(selector);
        currentValue = element.innerText.replace(/\xa0|\n/g, ' ');
        if (currentValue === expectedValue) {
          resolve(true);
          return;
        }

        const timerID = window.setTimeout(() => {
          observer.disconnect();
          reject(new Error(
            "Value not matched for '" + selector + "'.\n" +
            "Current: " + currentValue + "\n" +
            "Expected: " + expectedValue + ""));
        }, TIMEOUT_SECONDS * 1000);

        const observer = new MutationObserver(({}, observer) => {
          let element = document.querySelector(selector);
          if (!element) {
            return;
          }

          currentValue = element.innerText.replace(/\xa0|\n/g, ' ');
          if (currentValue === expectedValue) {
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
  )";

  auto result = EvalJs(context,
                       kWaitForElementToAppearScript +
                           content::JsReplace(script, selector, expectedValue),
                       content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                       content::ISOLATED_WORLD_ID_CONTENT_END);

  ASSERT_EQ(true, result);
}

void WaitForElementToContain(
    content::WebContents* context,
    const std::string& selector,
    const std::string& substring) {
  if (!context)
    return;

  auto script =
      kWaitForElementToAppearScript + content::JsReplace(R"(
          new Promise(async (resolve, reject) => {
            const TIMEOUT_SECONDS = 5;
            const selector = $1;
            const substring = $2;
            let currentText;

            try {
              let element = await waitForElementToAppear(selector);

              currentText = element.innerText.replace(/\xa0|\n/g, ' ');
              if (currentText.indexOf(substring) !== -1) {
                resolve(true);
                return;
              }

              const timerID = window.setTimeout(() => {
                observer.disconnect();
                reject(new Error(
                  "Substring not found in '" + selector + "'.\n" +
                  "Current text: " + currentText + "\n" +
                  "Expected substring: " + substring + ""));
              }, TIMEOUT_SECONDS * 1000);

              const observer = new MutationObserver(({}, observer) => {
                let element = document.querySelector(selector);
                if (!element) {
                  return;
                }

                currentText = element.innerText.replace(/\xa0|\n/g, ' ');
                if (currentText.indexOf(substring) !== -1) {
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
                                                         selector, substring);

  auto result = EvalJs(
      context,
      script,
      content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
      content::ISOLATED_WORLD_ID_CONTENT_END);

  ASSERT_EQ(true, result);
}

void WaitForElementToContainHTML(
    content::WebContents* context,
    const std::string& selector,
    const std::string& html) {
  if (!context)
    return;

  auto script = kWaitForElementToAppearScript +
      content::JsReplace(R"(
          new Promise(async (resolve, reject) => {
            const TIMEOUT_SECONDS = 5;
            const selector = $1;
            const expectedHTML = $2;
            let currentHTML;

            try {
              let element = await waitForElementToAppear(selector);

              currentHTML = element.innerHTML;
              if (currentHTML.indexOf(expectedHTML) !== -1) {
                resolve(true);
                return;
              }

              const timerID = window.setTimeout(() => {
                observer.disconnect();
                reject(new Error(
                  "HTML not found in '" + selector + "'.\n" +
                  "Current: " + currentHTML + "\n" +
                  "Expected: " + expectedHTML + ""));
              }, TIMEOUT_SECONDS * 1000);

              const observer = new MutationObserver(({}, observer) => {
                let element = document.querySelector(selector);
                if (!element) {
                  return;
                }

                currentHTML = element.innerHTML;
                if (currentHTML.indexOf(expectedHTML) !== -1) {
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
      html);

  auto result = EvalJs(
      context,
      script,
      content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
      content::ISOLATED_WORLD_ID_CONTENT_END);

  ASSERT_EQ(true, result);
}

void WaitForElementThenClick(
    content::WebContents* context,
    const std::string& selector) {
  if (!context)
    return;

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

std::string WaitForElementThenGetAttribute(
    content::WebContents* context,
    const std::string& selector,
    const std::string& attribute_name) {
  if (!context)
    return "";

  auto script = kWaitForElementToAppearScript +
    content::JsReplace(R"(
        new Promise(async (resolve, reject) => {
          try {
            const selector = $1;
            const attributeName = $2;

            const element = await waitForElementToAppear(selector);
            resolve(element.getAttribute(attributeName));
          } catch (error) {
            reject(error);
          }
        })
    )",
    selector,
    attribute_name);

  auto result = EvalJs(
    context,
    script,
    content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
    content::ISOLATED_WORLD_ID_CONTENT_END);

  return result.ExtractString();
}

std::string WaitForElementThenGetContent(
    content::WebContents* context,
    const std::string& selector) {
  if (!context)
    return "";

  auto script = kWaitForElementToAppearScript +
    content::JsReplace(R"(
        new Promise(async (resolve, reject) => {
          try {
            const selector = $1;

            const element = await waitForElementToAppear(selector);
            resolve(element.innerText);
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

  return result.ExtractString();
}

void DragAndDrop(
    content::WebContents* context,
    const std::string& drag_selector,
    const std::string& drop_selector) {
  if (!context)
    return;

  const std::string js_code = base::StringPrintf(
      R"(
        var triggerDragAndDrop = function (selectorDrag, selectorDrop) {

          // function for triggering mouse events
          var fireMouseEvent = function (type, elem, centerX, centerY) {
            var evt = document.createEvent('MouseEvents');
            evt.initMouseEvent(type, true, true, window, 1, 1, 1, centerX,
                               centerY, false, false, false, false, 0, elem);
            elem.dispatchEvent(evt);
          };

          // fetch target elements
          var elemDrag = document.querySelector(selectorDrag);
          var elemDrop = document.querySelector(selectorDrop);
          if (!elemDrag || !elemDrop) return false;

          // calculate positions
          var pos = elemDrag.getBoundingClientRect();
          var center1X = Math.floor((pos.left + pos.right) / 2);
          var center1Y = Math.floor((pos.top + pos.bottom) / 2);
          pos = elemDrop.getBoundingClientRect();
          var center2X = Math.floor((pos.left + pos.right) / 2);
          var center2Y = Math.floor((pos.top + pos.bottom) / 2);

          // mouse over dragged element and mousedown
          fireMouseEvent('mousemove', elemDrag, center1X, center1Y);
          fireMouseEvent('mouseenter', elemDrag, center1X, center1Y);
          fireMouseEvent('mouseover', elemDrag, center1X, center1Y);
          fireMouseEvent('mousedown', elemDrag, center1X, center1Y);

          // start dragging process over to drop target
          fireMouseEvent('dragstart', elemDrag, center1X, center1Y);
          fireMouseEvent('drag', elemDrag, center1X, center1Y);
          fireMouseEvent('mousemove', elemDrag, center1X, center1Y);
          fireMouseEvent('drag', elemDrag, center2X, center2Y);
          fireMouseEvent('mousemove', elemDrop, center2X, center2Y);

          // trigger dragging process on top of drop target
          fireMouseEvent('mouseenter', elemDrop, center2X, center2Y);
          fireMouseEvent('dragenter', elemDrop, center2X, center2Y);
          fireMouseEvent('mouseover', elemDrop, center2X, center2Y);
          fireMouseEvent('dragover', elemDrop, center2X, center2Y);

          // release dragged element on top of drop target
          fireMouseEvent('drop', elemDrop, center2X, center2Y);
          fireMouseEvent('dragend', elemDrag, center2X, center2Y);
          fireMouseEvent('mouseup', elemDrag, center2X, center2Y);

          return true;
        };

        triggerDragAndDrop(
          '%s',
          '%s')
      )",
      drag_selector.c_str(),
      drop_selector.c_str());
  content::EvalJsResult jsResult = EvalJs(
      context,
      js_code,
      content::EXECUTE_SCRIPT_NO_RESOLVE_PROMISES,
      content::ISOLATED_WORLD_ID_CONTENT_END);
  ASSERT_TRUE(jsResult.ExtractBool());
}

std::vector<double> GetSiteBannerTipOptions(content::WebContents* context) {
  if (!context)
    return {};

  WaitForElementToAppear(context, "[data-test-id=tip-amount-options]");
  auto options = content::EvalJs(
      context,
      R"(
          const delay = t => new Promise(resolve => setTimeout(resolve, t));
          delay(500).then(() => Array.from(
            document.querySelectorAll(
              "[data-test-id=tip-amount-options] [data-option-value]"
            )
          ).map(node => parseFloat(node.dataset.optionValue)))
      )",
      content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
      content::ISOLATED_WORLD_ID_CONTENT_END).ExtractList();

  std::vector<double> result;
  for (const auto& value : options) {
    result.push_back(value.GetDouble());
  }
  return result;
}

double GetRewardsPopupMonthlyTipValue(content::WebContents* context) {
  if (!context)
    return 0;

  WaitForElementToAppear(context, "[data-test-id=monthly-tip-button]");
  std::string script = R"_(
    new Promise(resolve => setTimeout(resolve, 0)).then(() => {
      const elem = document.querySelector(
        '[data-test-id=monthly-tip-button]')
      return elem && parseFloat(elem.innerText) || 0
    })
  )_";
  return content::EvalJs(context, script,
                         content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                         content::ISOLATED_WORLD_ID_CONTENT_END)
      .ExtractDouble();
}

}  // namespace brave_rewards::test_util
