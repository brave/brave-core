// This file contains code that used to be upstream and had to be restored in
// Brave to support delta updates on Windows until we are on Omaha 4. See:
// github.com/brave/brave-core/pull/31937

#include "chrome/installer/util/google_update_settings.h"

#define UpdateInstallStatus UpdateInstallStatus_Unused
#define UpdateGoogleUpdateApKey UpdateGoogleUpdateApKey_Unused
#include <chrome/installer/util/google_update_settings.cc>
#undef UpdateGoogleUpdateApKey
#undef UpdateInstallStatus

void GoogleUpdateSettings::UpdateInstallStatus(
    bool system_install,
    installer::ArchiveType archive_type,
    int install_return_code) {
  DCHECK(archive_type != installer::UNKNOWN_ARCHIVE_TYPE ||
         install_return_code != 0);

  installer::AdditionalParameters additional_parameters;
  if (UpdateGoogleUpdateApKey(archive_type, install_return_code,
                              &additional_parameters) &&
      !additional_parameters.Commit()) {
    PLOG(ERROR) << "Failed to write to application's ClientState key "
                << google_update::kRegApField << " = "
                << additional_parameters.value();
  }
}

bool GoogleUpdateSettings::UpdateGoogleUpdateApKey(
    installer::ArchiveType archive_type,
    int install_return_code,
    installer::AdditionalParameters* additional_parameters) {
  DCHECK(archive_type != installer::UNKNOWN_ARCHIVE_TYPE ||
         install_return_code != 0);
  bool modified = false;

  if (archive_type == installer::FULL_ARCHIVE_TYPE || !install_return_code) {
    if (additional_parameters->SetFullSuffix(false)) {
      VLOG(1) << "Removed incremental installer failure key; "
                 "switching to channel: "
              << additional_parameters->value();
      modified = true;
    }
  } else if (archive_type == installer::INCREMENTAL_ARCHIVE_TYPE) {
    if (additional_parameters->SetFullSuffix(true)) {
      VLOG(1) << "Incremental installer failed; switching to channel: "
              << additional_parameters->value();
      modified = true;
    } else {
      VLOG(1) << "Incremental installer failure; already on channel: "
              << additional_parameters->value();
    }
  } else {
    // It's okay if we don't know the archive type.  In this case, leave the
    // "-full" suffix as we found it.
    DCHECK_EQ(installer::UNKNOWN_ARCHIVE_TYPE, archive_type);
  }

  return modified;
}
