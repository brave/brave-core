#include "chrome/browser/permissions/permission_util.h"
#define GetPermissionString GetPermissionString_ChromiumImpl
#define GetRequestType GetRequestType_ChromiumImpl
#define GetPermissionType GetPermissionType_ChromiumImpl
#define IsPermission IsPermission_ChromiumImpl
#include "../../../../../../chrome/browser/permissions/permission_util.cc"
#undef IsPermission
#undef GetPermissionType
#undef GetRequestType
#undef GetPermissionString

std::string PermissionUtil::GetPermissionString(
    ContentSettingsType content_type) {
  if (content_type == CONTENT_SETTINGS_TYPE_AUTOPLAY)
    return "Autoplay";
  return GetPermissionString_ChromiumImpl(content_type);
}

PermissionRequestType PermissionUtil::GetRequestType(ContentSettingsType type) {
    if (type == CONTENT_SETTINGS_TYPE_AUTOPLAY)
      return PermissionRequestType::PERMISSION_AUTOPLAY;
    return GetRequestType_ChromiumImpl(type);
}

bool PermissionUtil::GetPermissionType(ContentSettingsType type,
                                       PermissionType* out) {
  if (type == CONTENT_SETTINGS_TYPE_AUTOPLAY) {
    *out = PermissionType::AUTOPLAY;
    return true;
  }
  return GetPermissionType_ChromiumImpl(type, out);
}

bool PermissionUtil::IsPermission(ContentSettingsType type) {
  if (type == CONTENT_SETTINGS_TYPE_AUTOPLAY)
    return true;
  return IsPermission_ChromiumImpl(type);
}
