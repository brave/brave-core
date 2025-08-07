/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_account/allow_brave_account_tag.h"

void AllowBraveAccountTag::Mark(content::WebContents* web_contents) {
  AllowBraveAccountTag::CreateForWebContents(web_contents);
}

bool AllowBraveAccountTag::IsSet(content::WebContents* web_contents) {
  return AllowBraveAccountTag::FromWebContents(web_contents);
}

AllowBraveAccountTag::~AllowBraveAccountTag() = default;

WEB_CONTENTS_USER_DATA_KEY_IMPL(AllowBraveAccountTag);
