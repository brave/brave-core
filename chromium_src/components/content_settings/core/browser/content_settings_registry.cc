#include "../../../../../../components/content_settings/core/browser/content_settings_registry.cc"

namespace content_settings {

void ContentSettingsRegistry::BraveInit() {
  // Add CONTENT_SETTING_ASK and make it default for autoplay
  content_settings_info_.erase(CONTENT_SETTINGS_TYPE_AUTOPLAY);
  website_settings_registry_->UnRegister(CONTENT_SETTINGS_TYPE_AUTOPLAY);
  Register(CONTENT_SETTINGS_TYPE_AUTOPLAY, "autoplay", CONTENT_SETTING_ASK,
           WebsiteSettingsInfo::UNSYNCABLE, WhitelistedSchemes(),
           ValidSettings(CONTENT_SETTING_ALLOW, CONTENT_SETTING_BLOCK,
                         CONTENT_SETTING_ASK),
           WebsiteSettingsInfo::REQUESTING_ORIGIN_ONLY_SCOPE,
           WebsiteSettingsRegistry::DESKTOP |
               WebsiteSettingsRegistry::PLATFORM_ANDROID,
           ContentSettingsInfo::INHERIT_IN_INCOGNITO);

}

}
