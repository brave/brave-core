/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Based on Chromium code subject to the following license:
// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_SETTINGS_COOKIES_VIEW_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_SETTINGS_COOKIES_VIEW_HANDLER_H_

#include <memory>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include "base/gtest_prod_util.h"
#include "chrome/browser/browsing_data/cookies_tree_model.h"
#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"

class CookiesTreeModelUtil;

namespace settings {

class CookiesViewHandler : public SettingsPageUIHandler,
                           public CookiesTreeModel::Observer {
 public:
  CookiesViewHandler();

  CookiesViewHandler(const CookiesViewHandler&) = delete;
  CookiesViewHandler& operator=(const CookiesViewHandler&) = delete;

  ~CookiesViewHandler() override;

  // SettingsPageUIHandler:
  void OnJavascriptAllowed() override;
  void OnJavascriptDisallowed() override;
  void RegisterMessages() override;

  // CookiesTreeModel::Observer:
  void TreeNodeAdded(ui::TreeModel* model,
                     ui::TreeModelNode* parent,
                     size_t index) override;
  void TreeNodeRemoved(ui::TreeModel* model,
                       ui::TreeModelNode* parent,
                       size_t index) override;
  void TreeModelBeginBatchDeprecated(CookiesTreeModel* model) override;
  void TreeModelEndBatchDeprecated(CookiesTreeModel* model) override;

  // Sets the tree model that will be used when the handler creates a tree
  // model, instead of building from from the profile.
  void SetCookiesTreeModelForTesting(
      std::unique_ptr<CookiesTreeModel> cookies_tree_model);

 private:
  friend class CookiesViewHandlerTest;
  FRIEND_TEST_ALL_PREFIXES(CookiesViewHandlerTest, ImmediateTreeOperation);
  FRIEND_TEST_ALL_PREFIXES(CookiesViewHandlerTest, HandleGetCookieDetails);
  FRIEND_TEST_ALL_PREFIXES(CookiesViewHandlerTest, HandleRemoveAll);
  FRIEND_TEST_ALL_PREFIXES(CookiesViewHandlerTest, HandleRemoveItem);
  FRIEND_TEST_ALL_PREFIXES(CookiesViewHandlerTest, HandleRemoveSite);

  // Recreates the CookiesTreeModel and resets the current |filter_|.
  void RecreateCookiesTreeModel();

  // Remove selected sites data.
  void HandleRemoveSite(const base::Value::List& args);
  void RemoveSite(const std::u16string& site);

  // Retrieve cookie details for a specific site.
  void HandleGetCookieDetails(const base::Value::List& args);
  void GetCookieDetails(const std::string& callback_id,
                        const std::string& site);
  void HandleReloadCookies(const base::Value::List& args);
  // Remove all sites data.
  void HandleRemoveAll(const base::Value::List& args);
  void RemoveAll(const std::string& callback_id);

  // Remove a single item.
  void HandleRemoveItem(const base::Value::List& args);
  void RemoveItem(const std::string& path);

  // Flag to indicate whether there is a batch update in progress.
  bool batch_update_;

  // The Cookies Tree model
  std::unique_ptr<CookiesTreeModel> cookies_tree_model_;

  // Cookies tree model which can be set for testing and will be used instead
  // of creating one directly from the profile.
  std::unique_ptr<CookiesTreeModel> cookies_tree_model_for_testing_;

  // Only show items that contain |filter|.
  std::u16string filter_;

  struct Request {
    // Specifies the batch behavior of the tree model when this request is run
    // against it. Batch behavior must be constant across invocations, and
    // defines when tasks can be queued.
    enum TreeModelBatchBehavior {
      // The request will not cause a batch operation to be started. Tasks may
      // only be queued when the request is first processed.
      NO_BATCH,

      // The request will cause a batch to start and finish syncronously. Tasks
      // may only be queued when the request is first processed.
      SYNC_BATCH,

      // The request will cause an asynchronous batch update to be run. Both
      // batch end and begin may occur asynchronously. Tasks may be queued when
      // the request is first processed, and when the batch is finished.
      ASYNC_BATCH
    };

    // Creates a request with a task to be queued when the request is first
    // processed.
    Request(TreeModelBatchBehavior batch_behavior,
            base::OnceClosure initial_task);

    // Creates a request with both a task to be queued when processed, and a
    // task to be queued when the tree model batch finishes. This constructor
    // implies |batch_behavior| == ASYNC_BATCH.
    Request(base::OnceClosure initial_task, base::OnceClosure batch_end_task);

    ~Request();
    Request(const Request&) = delete;
    Request(Request&& other);
    Request& operator=(const Request&&) = delete;

    TreeModelBatchBehavior batch_behavior;

    // Task which is run when the request reaches the front of the queue.
    // Task must only interact with the tree model in a synchronous manner.
    base::OnceClosure initial_task;

    // Optional task which is queued to run when the tree model batch ends.
    // Only valid when |batch_behavior| == ASYNC_BATCH. Must only interact with
    // the tree model in a synchronous manner.
    base::OnceClosure batch_end_task;
  };

  // The current client requests.
  std::queue<Request> pending_requests_;
  bool request_in_progress_ = false;

  // Check the request queue and process the first request if approproiate.
  void ProcessPendingRequests();

  // Signal that the request at the head of the request queue is complete.
  void RequestComplete();

  // Sorted index list, by site. Indexes refer to |model->GetRoot()| children.
  typedef std::pair<std::u16string, size_t> LabelAndIndex;
  std::vector<LabelAndIndex> sorted_sites_;

  std::unique_ptr<CookiesTreeModelUtil> model_util_;

  // Used to cancel callbacks when JavaScript becomes disallowed.
  base::WeakPtrFactory<CookiesViewHandler> callback_weak_ptr_factory_{this};
};

}  // namespace settings

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_SETTINGS_COOKIES_VIEW_HANDLER_H_
