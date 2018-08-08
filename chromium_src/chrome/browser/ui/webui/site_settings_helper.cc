#define HasRegisteredGroupName HasRegisteredGroupName_ChromiumImpl
#define ContentSettingsTypeFromGroupName ContentSettingsTypeFromGroupName_ChromiumImpl
#define ContentSettingsTypeToGroupName ContentSettingsTypeToGroupName_ChromiumImpl
#include "../../../../../../chrome/browser/ui/webui/site_settings_helper.cc"
#undef ContentSettingsTypeToGroupName
#undef ContentSettingsTypeFromGroupName
#undef HasRegisteredGroupName

namespace site_settings {

bool HasRegisteredGroupName(ContentSettingsType type) {
  if (type == CONTENT_SETTINGS_TYPE_AUTOPLAY)
    return true;
  return HasRegisteredGroupName_ChromiumImpl(type);
}

ContentSettingsType ContentSettingsTypeFromGroupName(const std::string& name) {
  if (name == "autoplay")
    return CONTENT_SETTINGS_TYPE_AUTOPLAY;
  return ContentSettingsTypeFromGroupName_ChromiumImpl(name);
}

std::string ContentSettingsTypeToGroupName(ContentSettingsType type) {
  if (type == CONTENT_SETTINGS_TYPE_AUTOPLAY)
    return "autoplay";
  return ContentSettingsTypeToGroupName_ChromiumImpl(type);
}

} // namespace site_settings
