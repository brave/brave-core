/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/permissions/mock_permission_lifetime_prompt_factory.h"

#include "base/bind.h"
#include "base/containers/contains.h"
#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "base/stl_util.h"
#include "components/permissions/permission_request_manager.h"
#include "components/permissions/request_type.h"
#include "components/permissions/test/mock_permission_prompt.h"
#include "content/public/browser/web_contents.h"

namespace permissions {

MockPermissionLifetimePromptFactory::MockPermissionLifetimePromptFactory(
    PermissionRequestManager* manager)
    : show_count_(0),
      response_type_(PermissionRequestManager::NONE),
      manager_(manager) {
  manager->set_view_factory_for_testing(base::BindRepeating(
      &MockPermissionLifetimePromptFactory::Create, base::Unretained(this)));
}

MockPermissionLifetimePromptFactory::~MockPermissionLifetimePromptFactory() {
  for (auto* prompt : prompts_)
    prompt->ResetFactory();
  prompts_.clear();
}

std::unique_ptr<PermissionPrompt> MockPermissionLifetimePromptFactory::Create(
    content::WebContents* web_contents,
    PermissionPrompt::Delegate* delegate) {
  auto prompt = std::make_unique<MockPermissionLifetimePrompt>(this, delegate);

  prompts_.push_back(prompt.get());
  show_count_++;

  if (!show_bubble_quit_closure_.is_null())
    show_bubble_quit_closure_.Run();

  manager_->set_auto_response_for_test(response_type_);

  OnPermissionPromptCreated(prompt.get());
  return prompt;
}

bool MockPermissionLifetimePromptFactory::IsVisible() const {
  return !prompts_.empty();
}

void MockPermissionLifetimePromptFactory::WaitForPermissionBubble() {
  if (IsVisible())
    return;
  DCHECK(show_bubble_quit_closure_.is_null());
  base::RunLoop loop;
  show_bubble_quit_closure_ = loop.QuitClosure();
  loop.Run();
  show_bubble_quit_closure_ = base::RepeatingClosure();
}

void MockPermissionLifetimePromptFactory::HideView(
    MockPermissionLifetimePrompt* prompt) {
  base::Erase(prompts_, prompt);
}

}  // namespace permissions
