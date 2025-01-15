// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// based on //chrome/browser/ui/webui/whats_new/whats_new_handler.cc

#include "brave/browser/ui/webui/brave_education/brave_education_handler.h"

BraveEducationHandler::BraveEducationHandler(
    mojo::PendingReceiver<brave_education::mojom::PageHandler> receiver,
    mojo::PendingRemote<brave_education::mojom::Page> page,
    brave_education::EducationPageType page_type)
    : receiver_(this, std::move(receiver)),
      page_(std::move(page)),
      page_type_(page_type) {}

BraveEducationHandler::~BraveEducationHandler() = default;

void BraveEducationHandler::GetServerUrl(GetServerUrlCallback callback) {
  std::move(callback).Run(
      brave_education::GetEducationPageServerURL(page_type_));
}

void BraveEducationHandler::RecordBrowserCommandExecuted() {}
