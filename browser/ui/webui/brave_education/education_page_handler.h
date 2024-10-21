/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_EDUCATION_EDUCATION_PAGE_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_EDUCATION_EDUCATION_PAGE_HANDLER_H_

#include <string>

#include "brave/components/brave_education/common/education_page.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace brave_education {

class EducationPageHandler : public mojom::EducationPageHandler {
 public:
  explicit EducationPageHandler(
      mojo::PendingReceiver<mojom::EducationPageHandler> receiver);

  ~EducationPageHandler() override;

  // mojom::EducationPageHandler:
  void GetServerUrl(const std::string& webui_url,
                    GetServerUrlCallback callback) override;

 private:
  mojo::Receiver<mojom::EducationPageHandler> receiver_;
};

}  // namespace brave_education

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_EDUCATION_EDUCATION_PAGE_HANDLER_H_
