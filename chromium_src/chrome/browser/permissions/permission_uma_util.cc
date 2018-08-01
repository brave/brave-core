#include "chrome/browser/permissions/permission_util.h"

#include "chrome/browser/permissions/permission_request.h"

namespace {

std::string GetPermissionRequestString_ChromiumImpl(PermissionRequestType type);
void BraveRecordPermissionAction (ContentSettingsType permission,
                                  bool secure_origin,
                                  PermissionAction action);

std::string GetPermissionRequestString(PermissionRequestType type) {
  if (type == PermissionRequestType::PERMISSION_AUTOPLAY)
    return "Autoplay";
  return GetPermissionRequestString_ChromiumImpl(type);
}

}

#include "../../../../../chrome/browser/permissions/permission_uma_util.cc"

namespace {

void BraveRecordPermissionAction (ContentSettingsType permission,
                                  bool secure_origin,
                                  PermissionAction action) {
  switch (permission) {
    case CONTENT_SETTINGS_TYPE_AUTOPLAY:
      PERMISSION_ACTION_UMA(secure_origin, "Permissions.Action.Autoplay",
                            "Permissions.Action.SecureOrigin.Autoplay",
                            "Permissions.Action.InsecureOrigin.Autoplay",
                            action);
      break;
    default:
      break;
  }
}

}
