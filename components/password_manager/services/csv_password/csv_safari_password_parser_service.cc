// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/password_manager/services/csv_password/csv_safari_password_parser_service.h"

#include "components/strings/grit/components_strings.h"
#include "content/public/browser/service_process_host.h"

namespace password_manager {

mojo::Remote<password_manager::mojom::CSVSafariPasswordParser>
LaunchCSVSafariPasswordParser() {
  return content::ServiceProcessHost::Launch<
      password_manager::mojom::CSVSafariPasswordParser>(
      content::ServiceProcessHost::Options()
          .WithDisplayName(
              IDS_PASSWORD_MANAGER_CSV_PASSWORD_PARSER_SERVICE_DISPLAY_NAME)
          .Pass());
}

}  // namespace password_manager
