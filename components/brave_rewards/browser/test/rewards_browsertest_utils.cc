/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/test/rewards_browsertest_utils.h"

#include "content/public/test/browser_test_utils.h"

namespace rewards_browsertest_utils {

static const char kWaitForElementToAppearScript[] = R"(
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

void WaitForElementToAppear(
    content::WebContents* context,
    const std::string& selector,
    bool should_appear) {
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

void WaitForElementToContain(
    content::WebContents* context,
    const std::string& selector,
    const std::string& substring) {
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

void WaitForElementToContainHTML(
    content::WebContents* context,
    const std::string& selector,
    const std::string& html) {
  auto script = kWaitForElementToAppearScript +
      content::JsReplace(R"(
          new Promise(async (resolve, reject) => {
            const TIMEOUT_SECONDS = 5;
            const selector = $1;
            const html = $2;

            try {
              let element = await waitForElementToAppear(selector);

              if (element.innerHTML.indexOf(html) !== -1) {
                resolve(true);
                return;
              }

              const timerID = window.setTimeout(() => {
                observer.disconnect();
                reject(new Error("Timed out waiting for '" + selector + "' " +
                    "to contain '" + html + "'"));
              }, TIMEOUT_SECONDS * 1000);

              const observer = new MutationObserver(({}, observer) => {
                let element = document.querySelector(selector);
                if (!element) {
                  return;
                }

                if (element.innerHTML.indexOf(html) !== -1) {
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

void IsMediaTipsInjected(content::WebContents* context, bool should_appear) {
  WaitForElementToAppear(context, ".action-brave-tip", should_appear);
}

}  // namespace rewards_browsertest_utils
