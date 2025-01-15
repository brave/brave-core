// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// based on //chrome/browser/ui/webui/whats_new/whats_new_handler.h

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_EDUCATION_BRAVE_EDUCATION_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_EDUCATION_BRAVE_EDUCATION_HANDLER_H_

#include "brave/components/brave_education/education_urls.h"
#include "brave/browser/ui/webui/brave_education/brave_education.mojom.h"
// #include "components/user_education/webui/whats_new_registry.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "ui/webui/mojo_web_ui_controller.h"

class Profile;

// Page handler for brave://getting-started
class BraveEducationHandler : public brave_education::mojom::PageHandler {
 public:
  BraveEducationHandler(
      mojo::PendingReceiver<brave_education::mojom::PageHandler> receiver,
      mojo::PendingRemote<brave_education::mojom::Page> page,
      brave_education::EducationPageType page_type);
  ~BraveEducationHandler() override;
  BraveEducationHandler(const BraveEducationHandler&) = delete;
  BraveEducationHandler& operator=(const BraveEducationHandler&) = delete;

 private:
  // brave_education::mojom::PageHandler
  void GetServerUrl(GetServerUrlCallback callback) override;
  void RecordBrowserCommandExecuted() override;

  // These are located at the end of the list of member variables to ensure the
  // WebUI page is disconnected before other members are destroyed.
  mojo::Receiver<brave_education::mojom::PageHandler> receiver_;
  mojo::Remote<brave_education::mojom::Page> page_;
  brave_education::EducationPageType page_type_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_EDUCATION_BRAVE_EDUCATION_HANDLER_H_
