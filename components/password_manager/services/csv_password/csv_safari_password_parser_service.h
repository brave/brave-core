// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PASSWORD_MANAGER_SERVICES_CSV_PASSWORD_CSV_SAFARI_PASSWORD_PARSER_SERVICE_H_
#define BRAVE_COMPONENTS_PASSWORD_MANAGER_SERVICES_CSV_PASSWORD_CSV_SAFARI_PASSWORD_PARSER_SERVICE_H_

#include "brave/components/password_manager/services/csv_password/public/mojom/csv_safari_password_parser.mojom.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace password_manager {

// Launches a new instance of the CSVSafariPasswordParser service in an
// isolated, sandboxed process, and returns a remote interface to control the
// service. The lifetime of the process is tied to that of the Remote. May be
// called from any thread.
mojo::Remote<password_manager::mojom::CSVSafariPasswordParser>
LaunchCSVSafariPasswordParser();

}  // namespace password_manager

#endif  // BRAVE_COMPONENTS_PASSWORD_MANAGER_SERVICES_CSV_PASSWORD_CSV_SAFARI_PASSWORD_PARSER_SERVICE_H_
