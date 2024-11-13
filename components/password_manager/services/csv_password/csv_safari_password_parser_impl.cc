// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/password_manager/services/csv_password/csv_safari_password_parser_impl.h"

#include <utility>
#include <vector>

#include "brave/components/password_manager/core/browser/import/csv_safari_password.h"
#include "brave/components/password_manager/core/browser/import/csv_safari_password_sequence.h"

namespace password_manager {

CSVSafariPasswordParserImpl::CSVSafariPasswordParserImpl(
    mojo::PendingReceiver<mojom::CSVSafariPasswordParser> receiver)
    : receiver_(this, std::move(receiver)) {}

CSVSafariPasswordParserImpl::~CSVSafariPasswordParserImpl() = default;

void CSVSafariPasswordParserImpl::ParseCSV(const std::string& raw_json,
                                           ParseCSVCallback callback) {
  mojom::CSVSafariPasswordSequencePtr result = nullptr;
  CSVSafariPasswordSequence seq(raw_json);
  if (seq.result() == CSVSafariPassword::Status::kOK) {
    result = mojom::CSVSafariPasswordSequence::New();
    for (const auto& pwd : seq) {
      result->csv_passwords.push_back(pwd);
    }
  }
  std::move(callback).Run(std::move(result));
}

}  // namespace password_manager
