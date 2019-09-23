#include "brave/browser/autoplay/autoplay_permission_context.h"
#include "brave/browser/geolocation/brave_geolocation_permission_context.h"
#include "components/content_settings/core/common/content_settings.h"
#include "content/public/browser/permission_type.h"

using content::PermissionType;

namespace {

ContentSettingsType PermissionTypeToContentSettingSafe_ChromiumImpl(
    PermissionType permission);

ContentSettingsType PermissionTypeToContentSettingSafe(
    PermissionType permission) {
  if (permission == PermissionType::AUTOPLAY)
    return CONTENT_SETTINGS_TYPE_AUTOPLAY;
  return PermissionTypeToContentSettingSafe_ChromiumImpl(permission);
}

} // namespace

#define GeolocationPermissionContext BraveGeolocationPermissionContext
#define PermissionManagerFactory BravePermissionManagerFactory
#include "../../../../../chrome/browser/permissions/permission_manager.cc"
#undef GeolocationPermissionContext
