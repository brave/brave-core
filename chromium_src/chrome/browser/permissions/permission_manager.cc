#include "brave/browser/autoplay/autoplay_permission_context.h"
#include "components/content_settings/core/common/content_settings.h"
#include "content/public/browser/permission_type.h"

using content::PermissionType;

namespace {

ContentSettingsType PermissionTypeToContentSetting_ChromiumImpl(PermissionType permission);

ContentSettingsType PermissionTypeToContentSetting(PermissionType permission) {
  if (permission == PermissionType::AUTOPLAY)
    return CONTENT_SETTINGS_TYPE_AUTOPLAY;
  return PermissionTypeToContentSetting_ChromiumImpl(permission);
}

} // namespace

#define PermissionManagerFactory BravePermissionManagerFactory
#include "../../../../../chrome/browser/permissions/permission_manager.cc"
