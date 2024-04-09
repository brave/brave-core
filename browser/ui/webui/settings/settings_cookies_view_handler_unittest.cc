/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Based on Chromium code subject to the following license:
// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/ui/webui/settings/settings_cookies_view_handler.h"

#include <memory>
#include <string>

#include "base/test/bind.h"
#include "base/values.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "components/browsing_data/content/mock_cookie_helper.h"
#include "components/browsing_data/content/mock_local_storage_helper.h"
#include "content/public/test/test_web_ui.h"
#include "third_party/blink/public/common/storage_key/storage_key.h"

namespace {

constexpr char kCallbackId[] = "test-callback-id";
constexpr char kTestOrigin1[] = "https://a-example.com";
constexpr char kTestOrigin2[] = "https://b-example.com";
constexpr char kTestHost1[] = "a-example.com";
constexpr char kTestCookie1[] = "A=1";
constexpr char kTestCookie2[] = "B=1";

}  // namespace

namespace settings {

class CookiesViewHandlerTest : public ChromeRenderViewHostTestHarness {
 public:
  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();

    web_ui_ = std::make_unique<content::TestWebUI>();
    web_ui_->set_web_contents(web_contents());
    handler_ = std::make_unique<CookiesViewHandler>();
    handler_->set_web_ui(web_ui());
    web_ui_->ClearTrackedCalls();
  }

  void TearDown() override {
    handler_->set_web_ui(nullptr);
    handler_.reset();
    web_ui_.reset();

    mock_browsing_data_local_storage_helper_ = nullptr;
    mock_browsing_data_cookie_helper_ = nullptr;

    ChromeRenderViewHostTestHarness::TearDown();
  }

  void SetupTreeModelForTesting() {
    mock_browsing_data_cookie_helper_ =
        base::MakeRefCounted<browsing_data::MockCookieHelper>(
            profile()->GetDefaultStoragePartition());
    mock_browsing_data_local_storage_helper_ =
        base::MakeRefCounted<browsing_data::MockLocalStorageHelper>(
            profile()->GetDefaultStoragePartition());

    auto container = std::make_unique<LocalDataContainer>(
        mock_browsing_data_cookie_helper_,
        mock_browsing_data_local_storage_helper_,
        /*session_storage_helper=*/nullptr,
        /*quota_helper=*/nullptr);
    auto mock_cookies_tree_model = std::make_unique<CookiesTreeModel>(
        std::move(container), profile()->GetExtensionSpecialStoragePolicy());

    mock_browsing_data_local_storage_helper_->AddLocalStorageForStorageKey(
        blink::StorageKey::CreateFromStringForTesting(kTestOrigin1), 2);
    mock_browsing_data_local_storage_helper_->AddLocalStorageForStorageKey(
        blink::StorageKey::CreateFromStringForTesting(kTestOrigin2), 3);

    mock_browsing_data_cookie_helper_->AddCookieSamples(GURL(kTestOrigin1),
                                                        kTestCookie1);
    mock_browsing_data_cookie_helper_->AddCookieSamples(GURL(kTestOrigin2),
                                                        kTestCookie2);

    handler()->SetCookiesTreeModelForTesting(
        std::move(mock_cookies_tree_model));
  }

  void NotifyTreeModel() {
    mock_browsing_data_local_storage_helper_->Notify();
    mock_browsing_data_cookie_helper_->Notify();
  }

  void SetupHandlerWithTreeModel() {
    SetupTreeModelForTesting();

    base::Value::List reload_args;
    reload_args.Append(kCallbackId);
    handler()->HandleReloadCookies(reload_args);

    task_environment()->RunUntilIdle();
    NotifyTreeModel();

    // After batch end, the handler will have posted a task to complete
    // the callback.
    task_environment()->RunUntilIdle();
    const content::TestWebUI::CallData& data = *web_ui()->call_data().back();
    EXPECT_EQ("cr.webUIResponse", data.function_name());
    EXPECT_EQ(kCallbackId, data.arg1()->GetString());
    ASSERT_TRUE(data.arg2()->GetBool());

    web_ui_->ClearTrackedCalls();
  }

  content::TestWebUI* web_ui() { return web_ui_.get(); }
  CookiesViewHandler* handler() { return handler_.get(); }

 private:
  std::unique_ptr<content::TestWebUI> web_ui_;
  std::unique_ptr<CookiesViewHandler> handler_;

  // Ref pointers to storage helpers used in the tree model used for testing.
  // Retained to allow control over batch update completion.
  scoped_refptr<browsing_data::MockLocalStorageHelper>
      mock_browsing_data_local_storage_helper_;
  scoped_refptr<browsing_data::MockCookieHelper>
      mock_browsing_data_cookie_helper_;
};

TEST_F(CookiesViewHandlerTest, ImmediateTreeOperation) {
  // Check that a query which assumes a tree model to have been created
  // previously results in a tree being created before the request is handled.
  SetupTreeModelForTesting();

  base::Value::List args;
  args.Append(kCallbackId);
  args.Append(kTestHost1);
  handler()->HandleGetCookieDetails(args);
  task_environment()->RunUntilIdle();

  // At this point the handler should have queued the creation of a tree and
  // be awaiting batch completion.
  NotifyTreeModel();
  task_environment()->RunUntilIdle();

  // Check that the returned information is accurate, despite not having
  // previously loaded the tree.
  const content::TestWebUI::CallData& data = *web_ui()->call_data().back();
  EXPECT_EQ(kCallbackId, data.arg1()->GetString());
  EXPECT_EQ("cr.webUIResponse", data.function_name());
  ASSERT_TRUE(data.arg2()->GetBool());

  const base::Value::List& cookies_list = data.arg3()->GetList();
  ASSERT_EQ(2UL, cookies_list.size());
  EXPECT_EQ("cookie", *cookies_list[0].GetDict().FindString("type"));
  EXPECT_EQ("local_storage", *cookies_list[1].GetDict().FindString("type"));
}

TEST_F(CookiesViewHandlerTest, HandleGetCookieDetails) {
  // Ensure that the cookie details are correctly returned for a site.
  SetupHandlerWithTreeModel();
  base::Value::List args;
  args.Append(kCallbackId);
  args.Append(kTestHost1);
  handler()->HandleGetCookieDetails(args);
  task_environment()->RunUntilIdle();

  const content::TestWebUI::CallData& data = *web_ui()->call_data().back();
  EXPECT_EQ(kCallbackId, data.arg1()->GetString());
  EXPECT_EQ("cr.webUIResponse", data.function_name());
  ASSERT_TRUE(data.arg2()->GetBool());

  const base::Value::List& cookies_list = data.arg3()->GetList();
  ASSERT_EQ(2UL, cookies_list.size());
  EXPECT_EQ("cookie", *cookies_list[0].GetDict().FindString("type"));
  EXPECT_EQ("local_storage", *cookies_list[1].GetDict().FindString("type"));
}

TEST_F(CookiesViewHandlerTest, HandleRemoveAll) {
  // Ensure that RemoveAll removes all cookies & storage.
  SetupHandlerWithTreeModel();
  {
    base::Value::List args;
    args.Append(kCallbackId);
    handler()->HandleRemoveAll(args);
    task_environment()->RunUntilIdle();

    const content::TestWebUI::CallData& data = *web_ui()->call_data().back();
    EXPECT_EQ(kCallbackId, data.arg1()->GetString());
    EXPECT_EQ("cr.webUIResponse", data.function_name());
    ASSERT_TRUE(data.arg2()->GetBool());
  }
}

TEST_F(CookiesViewHandlerTest, HandleRemoveItem) {
  // Delete an individual piece of site data. This requires first getting the
  // node path ID via the HandleGetCookieDetails function.
  SetupHandlerWithTreeModel();

  // Get the appropriate path for removal.
  std::string node_path_id;
  {
    base::Value::List args;
    args.Append(kCallbackId);
    args.Append(kTestHost1);
    handler()->HandleGetCookieDetails(args);
    task_environment()->RunUntilIdle();

    const content::TestWebUI::CallData& data = *web_ui()->call_data().back();
    const base::Value::List& cookies_list = data.arg3()->GetList();
    ASSERT_EQ(2UL, cookies_list.size());
    // Find the entry item associated with the kTestCookie1 cookie.
    for (const auto& cookie : cookies_list) {
      if (*cookie.GetDict().FindString("type") == "cookie") {
        node_path_id = *cookie.GetDict().FindString("idPath");
      }
    }
  }

  // Remove path and ensure that the removed item listener fires.
  {
    base::Value::List args;
    args.Append(node_path_id);
    handler()->HandleRemoveItem(args);
    task_environment()->RunUntilIdle();

    // Removal should fire an update event.
    const content::TestWebUI::CallData& all_data =
        *web_ui()->call_data().back();
    EXPECT_EQ("cr.webUIListenerCallback", all_data.function_name());
    EXPECT_EQ("on-tree-item-removed", all_data.arg1()->GetString());
  }

  // Ensure that the removed item is no longer present in cookie details.
  {
    base::Value::List args;
    args.Append(kCallbackId);
    args.Append(kTestHost1);
    handler()->HandleGetCookieDetails(args);
    task_environment()->RunUntilIdle();

    const content::TestWebUI::CallData& data = *web_ui()->call_data().back();
    const base::Value::List& cookies_list = data.arg3()->GetList();
    ASSERT_EQ(1UL, cookies_list.size());
    EXPECT_EQ("local_storage", *cookies_list[0].GetDict().FindString("type"));
  }
}

TEST_F(CookiesViewHandlerTest, HandleRemoveSite) {
  SetupHandlerWithTreeModel();

  // Check that removing a single site works.
  {
    base::Value::List args;
    args.Append(kTestHost1);
    handler()->HandleRemoveSite(args);
    task_environment()->RunUntilIdle();

    // Removal should fire an update event.
    const content::TestWebUI::CallData& data = *web_ui()->call_data().back();
    EXPECT_EQ("cr.webUIListenerCallback", data.function_name());
    EXPECT_EQ("on-tree-item-removed", data.arg1()->GetString());
  }
}

}  // namespace settings
