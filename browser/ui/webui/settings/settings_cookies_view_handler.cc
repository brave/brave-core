/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Based on Chromium code subject to the following license:
// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/ui/webui/settings/settings_cookies_view_handler.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/ranges/algorithm.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/cookies_tree_model_util.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_ui.h"

/*
 * Restored Chromium's site data details subpage
 * https://chromium.googlesource.com/chromium/src/+/
 * 22ecdb41cdc603d40b47f92addcd52933fdef445/chrome/browser/ui/webui/settings/
 * settings_cookies_view_handler.cc
 */

namespace settings {

CookiesViewHandler::Request::Request(TreeModelBatchBehavior batch_behavior,
                                     base::OnceClosure initial_task)
    : batch_behavior(batch_behavior), initial_task(std::move(initial_task)) {
  if (batch_behavior == Request::ASYNC_BATCH) {
    batch_end_task = base::DoNothing();
  }
}

CookiesViewHandler::Request::Request(base::OnceClosure initial_task,
                                     base::OnceClosure batch_end_task)
    : batch_behavior(Request::TreeModelBatchBehavior::ASYNC_BATCH),
      initial_task(std::move(initial_task)),
      batch_end_task(std::move(batch_end_task)) {}

CookiesViewHandler::Request::~Request() = default;

CookiesViewHandler::Request::Request(Request&& other) {
  initial_task = std::move(other.initial_task);
  batch_end_task = std::move(other.batch_end_task);
}

CookiesViewHandler::CookiesViewHandler()
    : batch_update_(false), model_util_(new CookiesTreeModelUtil) {}

CookiesViewHandler::~CookiesViewHandler() {}

void CookiesViewHandler::OnJavascriptAllowed() {
  // Some requests assume that a tree model has already been created, creating
  // here ensures this is true.
  pending_requests_.emplace(
      Request::ASYNC_BATCH,
      base::BindOnce(&CookiesViewHandler::RecreateCookiesTreeModel,
                     callback_weak_ptr_factory_.GetWeakPtr()));
  ProcessPendingRequests();
}

void CookiesViewHandler::OnJavascriptDisallowed() {
  callback_weak_ptr_factory_.InvalidateWeakPtrs();
  pending_requests_ = std::queue<Request>();
}

void CookiesViewHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "localData.removeAll",
      base::BindRepeating(&CookiesViewHandler::HandleRemoveAll,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "localData.removeItem",
      base::BindRepeating(&CookiesViewHandler::HandleRemoveItem,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "localData.getCookieDetails",
      base::BindRepeating(&CookiesViewHandler::HandleGetCookieDetails,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "localData.removeSite",
      base::BindRepeating(&CookiesViewHandler::HandleRemoveSite,
                          base::Unretained(this)));
}

void CookiesViewHandler::TreeNodeAdded(ui::TreeModel* model,
                                       ui::TreeModelNode* parent,
                                       size_t index) {}

void CookiesViewHandler::TreeNodeRemoved(ui::TreeModel* model,
                                         ui::TreeModelNode* parent,
                                         size_t index) {
  // Skip if there is a batch update in progress.
  if (batch_update_) {
    return;
  }
  FireWebUIListener("on-tree-item-removed");
}

void CookiesViewHandler::TreeModelBeginBatchDeprecated(
    CookiesTreeModel* model) {
  DCHECK(!batch_update_);  // There should be no nested batch begin.
  DCHECK(!pending_requests_.empty());
  batch_update_ = true;

  DCHECK_NE(Request::NO_BATCH, pending_requests_.front().batch_behavior);
}

void CookiesViewHandler::TreeModelEndBatchDeprecated(CookiesTreeModel* model) {
  DCHECK(batch_update_);
  DCHECK(!pending_requests_.empty());
  batch_update_ = false;

  DCHECK_NE(Request::NO_BATCH, pending_requests_.front().batch_behavior);

  if (pending_requests_.front().batch_behavior == Request::ASYNC_BATCH) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, std::move(pending_requests_.front().batch_end_task));

    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(&CookiesViewHandler::RequestComplete,
                                  callback_weak_ptr_factory_.GetWeakPtr()));
  }
}

void CookiesViewHandler::SetCookiesTreeModelForTesting(
    std::unique_ptr<CookiesTreeModel> cookies_tree_model) {
  cookies_tree_model_for_testing_ = std::move(cookies_tree_model);
}

void CookiesViewHandler::RecreateCookiesTreeModel() {
  cookies_tree_model_.reset();
  filter_.clear();
  cookies_tree_model_ = cookies_tree_model_for_testing_.get()
                            ? std::move(cookies_tree_model_for_testing_)
                            : CookiesTreeModel::CreateForProfileDeprecated(
                                  Profile::FromWebUI(web_ui()));
  cookies_tree_model_->AddCookiesTreeObserver(this);
}

void CookiesViewHandler::HandleGetCookieDetails(const base::Value::List& args) {
  CHECK_EQ(2U, args.size());
  std::string callback_id = args[0].GetString();
  std::string site = args[1].GetString();

  AllowJavascript();
  pending_requests_.emplace(
      Request::NO_BATCH, base::BindOnce(&CookiesViewHandler::GetCookieDetails,
                                        callback_weak_ptr_factory_.GetWeakPtr(),
                                        callback_id, site));
  ProcessPendingRequests();
}

void CookiesViewHandler::GetCookieDetails(const std::string& callback_id,
                                          const std::string& site) {
  const CookieTreeNode* node = model_util_->GetTreeNodeFromTitle(
      cookies_tree_model_->GetRoot(), base::UTF8ToUTF16(site));

  if (!node) {
    RejectJavascriptCallback(base::Value(callback_id), base::Value());
    return;
  }

  base::Value::List children = model_util_->GetChildNodeDetailsDeprecated(node);

  ResolveJavascriptCallback(base::Value(callback_id), children);
}

void CookiesViewHandler::HandleRemoveAll(const base::Value::List& args) {
  CHECK_EQ(1U, args.size());
  AllowJavascript();

  std::string callback_id = args[0].GetString();

  pending_requests_.emplace(
      Request::SYNC_BATCH,
      base::BindOnce(&CookiesViewHandler::RemoveAll,
                     callback_weak_ptr_factory_.GetWeakPtr(), callback_id));
  ProcessPendingRequests();
}

void CookiesViewHandler::RemoveAll(const std::string& callback_id) {
  cookies_tree_model_->DeleteAllStoredObjects();
  ResolveJavascriptCallback(base::Value(callback_id), base::Value());
}

void CookiesViewHandler::HandleRemoveItem(const base::Value::List& args) {
  std::string node_path = args[0].GetString();

  AllowJavascript();
  pending_requests_.emplace(
      Request::NO_BATCH,
      base::BindOnce(&CookiesViewHandler::RemoveItem,
                     callback_weak_ptr_factory_.GetWeakPtr(), node_path));
  ProcessPendingRequests();
}

void CookiesViewHandler::RemoveItem(const std::string& path) {
  const CookieTreeNode* node =
      model_util_->GetTreeNodeFromPath(cookies_tree_model_->GetRoot(), path);
  if (node) {
    cookies_tree_model_->DeleteCookieNode(const_cast<CookieTreeNode*>(node));
  }
}

void CookiesViewHandler::HandleRemoveSite(const base::Value::List& args) {
  CHECK_EQ(1U, args.size());
  std::u16string site = base::UTF8ToUTF16(args[0].GetString());
  AllowJavascript();
  pending_requests_.emplace(
      Request::NO_BATCH,
      base::BindOnce(&CookiesViewHandler::RemoveSite,
                     callback_weak_ptr_factory_.GetWeakPtr(), site));
  ProcessPendingRequests();
}

void CookiesViewHandler::RemoveSite(const std::u16string& site) {
  CookieTreeNode* parent = cookies_tree_model_->GetRoot();
  const auto i =
      base::ranges::find(parent->children(), site, &CookieTreeNode::GetTitle);
  if (i != parent->children().cend()) {
    cookies_tree_model_->DeleteCookieNode(i->get());
  }
}

void CookiesViewHandler::ProcessPendingRequests() {
  if (pending_requests_.empty()) {
    return;
  }

  // To ensure that multiple requests do not run during a tree model batch
  // update, only tasks for a single request are queued at any one time.
  if (request_in_progress_) {
    return;
  }

  request_in_progress_ = true;

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, std::move(pending_requests_.front().initial_task));
  if (pending_requests_.front().batch_behavior != Request::ASYNC_BATCH) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(&CookiesViewHandler::RequestComplete,
                                  callback_weak_ptr_factory_.GetWeakPtr()));
  }
}

void CookiesViewHandler::RequestComplete() {
  DCHECK(!pending_requests_.empty());
  DCHECK(!batch_update_);
  request_in_progress_ = false;
  pending_requests_.pop();
  ProcessPendingRequests();
}

void CookiesViewHandler::HandleReloadCookies(const base::Value::List& args) {
  CHECK_EQ(1U, args.size());
  std::string callback_id = args[0].GetString();

  // Allowing Javascript for the first time will queue a task to create a new
  // tree model. Thus the tree model only needs to be recreated if Javascript
  // has already been allowed. Reload cookies is often the first call made by
  // pages using this handler, so this avoids unnecessary work.
  if (IsJavascriptAllowed()) {
    pending_requests_.emplace(
        base::BindOnce(&CookiesViewHandler::RecreateCookiesTreeModel,
                       callback_weak_ptr_factory_.GetWeakPtr()),
        base::BindOnce(&CookiesViewHandler::ResolveJavascriptCallback,
                       callback_weak_ptr_factory_.GetWeakPtr(),
                       base::Value(callback_id), base::Value()));
  } else {
    AllowJavascript();
    pending_requests_.emplace(
        Request::NO_BATCH,
        base::BindOnce(&CookiesViewHandler::ResolveJavascriptCallback,
                       callback_weak_ptr_factory_.GetWeakPtr(),
                       base::Value(callback_id), base::Value()));
  }
  ProcessPendingRequests();
}

}  // namespace settings
