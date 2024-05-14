/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/tpcd/heuristics/redirect_heuristic_tab_helper.h"

#define RedirectHeuristicTabHelper RedirectHeuristicTabHelper_ChromiumImpl
#include "src/chrome/browser/tpcd/heuristics/redirect_heuristic_tab_helper.cc"
#undef RedirectHeuristicTabHelper

WEB_CONTENTS_USER_DATA_KEY_IMPL(RedirectHeuristicTabHelper);

RedirectHeuristicTabHelper::RedirectHeuristicTabHelper(
    content::WebContents* web_contents)
    : content::WebContentsUserData<RedirectHeuristicTabHelper>(*web_contents) {}
