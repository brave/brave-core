/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <vector>

#include "base/functional/bind.h"
#include "base/memory/ref_counted_memory.h"
#include "base/no_destructor.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "content/browser/webui/url_data_source_impl.h"
#include "content/browser/webui/web_ui_data_source_impl.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/url_data_source.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/content_client.h"
#include "url/gurl.h"

#define PopulateSharedResourcesDataSource \
  PopulateSharedResourcesDataSource_ChromiumImpl

#include "src/content/browser/webui/shared_resources_data_source.cc"

#undef PopulateSharedResourcesDataSource

namespace {

bool ShouldHandleWebUIRequestCallback(const std::string& path) {
  if (!base::EqualsCaseInsensitiveASCII(path, "fonts/poppins.css") &&
      !base::EqualsCaseInsensitiveASCII(path, "fonts/inter.css")) {
    return false;
  }

  static base::NoDestructor<std::string> language_code("");

  if (language_code->empty()) {
    const auto app_locale =
        content::GetContentClient()->browser()->GetApplicationLocale();

    const std::vector<std::string> locale_components = base::SplitString(
        app_locale, ".", base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
    if (locale_components.empty()) {
      return false;
    }

    std::string normalized_locale = locale_components.front();
    std::replace(normalized_locale.begin(), normalized_locale.end(), '-', '_');

    const std::vector<std::string> components = base::SplitString(
        normalized_locale, "_", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
    if (components.empty()) {
      return false;
    }

    *language_code = base::ToLowerASCII(components.front());
  }

  return *language_code == "ru" || *language_code == "el";
}

void HandleWebUIRequestCallback(
    content::WebUIDataSource* web_ui_data_source,
    const std::string& path,
    content::WebUIDataSource::GotDataCallback callback) {
  DCHECK(ShouldHandleWebUIRequestCallback(path));
  content::WebUIDataSourceImpl* web_ui_data_source_impl =
      static_cast<content::WebUIDataSourceImpl*>(web_ui_data_source);
  content::URLDataSourceImpl* url_data_source_impl =
      static_cast<content::URLDataSourceImpl*>(web_ui_data_source_impl);
  content::URLDataSource* url_data_source = url_data_source_impl->source();
  url_data_source->StartDataRequest(
      GURL("chrome://resources/fonts/manrope_as_poppins.css"),
      content::WebContents::Getter(), std::move(callback));
}

}  // namespace

namespace content {

void PopulateSharedResourcesDataSource(WebUIDataSource* source) {
  PopulateSharedResourcesDataSource_ChromiumImpl(source);
  source->SetRequestFilter(
      base::BindRepeating(&ShouldHandleWebUIRequestCallback),
      base::BindRepeating(&HandleWebUIRequestCallback, source));
}

}  // namespace content
