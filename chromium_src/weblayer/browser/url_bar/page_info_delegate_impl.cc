/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/weblayer/browser/url_bar/page_info_delegate_impl.cc"

bool weblayer::PageInfoDelegateImpl::BraveShouldShowPermission(
    ContentSettingsType type) {
  return true;
}
