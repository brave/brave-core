/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_EDUCATION_EDUCATION_PAGE_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_EDUCATION_EDUCATION_PAGE_HANDLER_H_

#include <memory>
#include <optional>
#include <string_view>
#include <utility>

#include "brave/components/brave_education/common/education_content_urls.h"
#include "brave/components/brave_education/common/education_page.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "ui/base/window_open_disposition.h"

class Profile;

namespace brave_education {

class EducationPageHandler : public mojom::EducationPageHandler {
 public:
  EducationPageHandler(
      mojo::PendingReceiver<mojom::EducationPageHandler> receiver,
      Profile* profile,
      std::optional<EducationContentType> content_type);

  ~EducationPageHandler() override;

  static constexpr std::string_view kChildSrcDirective =
      "child-src chrome://webui-test https://brave.com/;";

  // mojom::EducationPageHandler:
  void GetServerUrl(GetServerUrlCallback callback) override;

  void ExecuteCommand(mojom::Command command,
                      mojom::ClickInfoPtr click_info,
                      ExecuteCommandCallback callback) override;

  class Delegate {
   public:
    virtual ~Delegate() = default;

    virtual void OpenURL(const GURL& url,
                         WindowOpenDisposition disposition) = 0;
    virtual void OpenRewardsPanel() = 0;
    virtual void OpenVPNPanel() = 0;
    virtual void OpenAIChat() = 0;
  };

  void SetDelegateForTesting(std::unique_ptr<Delegate> delegate) {
    delegate_ = std::move(delegate);
  }

 private:
  bool CanExecute(mojom::Command command);

  mojo::Receiver<mojom::EducationPageHandler> receiver_;
  raw_ptr<Profile> profile_;
  std::optional<EducationContentType> content_type_;
  std::unique_ptr<Delegate> delegate_;
};

}  // namespace brave_education

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_EDUCATION_EDUCATION_PAGE_HANDLER_H_
