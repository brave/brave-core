/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_education/education_page_handler.h"

#include <utility>

#include "brave/components/brave_education/common/education_content_urls.h"

namespace brave_education {

EducationPageHandler::EducationPageHandler(
    mojo::PendingReceiver<mojom::EducationPageHandler> receiver)
    : receiver_(this, std::move(receiver)) {}

EducationPageHandler::~EducationPageHandler() = default;

void EducationPageHandler::GetServerUrl(const std::string& webui_url,
                                        GetServerUrlCallback callback) {
  if (auto content_type = EducationContentTypeFromBrowserURL(webui_url)) {
    auto server_url = GetEducationContentServerURL(*content_type).spec();
    std::move(callback).Run(server_url);
    return;
  }
  std::move(callback).Run("");
}

}  // namespace brave_education
