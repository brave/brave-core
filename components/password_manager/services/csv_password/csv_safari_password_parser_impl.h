// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PASSWORD_MANAGER_SERVICES_CSV_PASSWORD_CSV_SAFARI_PASSWORD_PARSER_IMPL_H_
#define BRAVE_COMPONENTS_PASSWORD_MANAGER_SERVICES_CSV_PASSWORD_CSV_SAFARI_PASSWORD_PARSER_IMPL_H_

#include <string>

#include "brave/components/password_manager/services/csv_password/public/mojom/csv_safari_password_parser.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace password_manager {

// Implementation of the CSVSafariPasswordParser mojom interface.
class CSVSafariPasswordParserImpl : public mojom::CSVSafariPasswordParser {
 public:
  // Constructs a CSVSafariPasswordParserImpl bound to |receiver|.
  explicit CSVSafariPasswordParserImpl(
      mojo::PendingReceiver<mojom::CSVSafariPasswordParser> receiver);
  ~CSVSafariPasswordParserImpl() override;
  CSVSafariPasswordParserImpl(const CSVSafariPasswordParserImpl&) = delete;
  CSVSafariPasswordParserImpl& operator=(const CSVSafariPasswordParserImpl&) =
      delete;

  // mojom::CSVSafariPasswordParser:
  void ParseCSV(const std::string& raw_json,
                ParseCSVCallback callback) override;

 private:
  mojo::Receiver<mojom::CSVSafariPasswordParser> receiver_;
};

}  // namespace password_manager

#endif  // BRAVE_COMPONENTS_PASSWORD_MANAGER_SERVICES_CSV_PASSWORD_CSV_SAFARI_PASSWORD_PARSER_IMPL_H_
