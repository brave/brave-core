#include "../../../../../../components/content_settings/core/browser/content_settings_registry.cc"

namespace content_settings {

void ContentSettingsRegistry::BraveInit() {
  // Add CONTENT_SETTING_ASK and make it default for autoplay
  content_settings_info_.erase(CONTENT_SETTINGS_TYPE_AUTOPLAY);
  website_settings_registry_->UnRegister(CONTENT_SETTINGS_TYPE_AUTOPLAY);
  Register(CONTENT_SETTINGS_TYPE_AUTOPLAY, "autoplay", CONTENT_SETTING_BLOCK,
           WebsiteSettingsInfo::UNSYNCABLE, WhitelistedSchemes(),
           ValidSettings(CONTENT_SETTING_ALLOW, CONTENT_SETTING_BLOCK,
                         CONTENT_SETTING_ASK),
           WebsiteSettingsInfo::SINGLE_ORIGIN_ONLY_SCOPE,
           WebsiteSettingsRegistry::DESKTOP |
               WebsiteSettingsRegistry::PLATFORM_ANDROID,
           ContentSettingsInfo::INHERIT_IN_INCOGNITO,
           ContentSettingsInfo::PERSISTENT);

  // Change plugins default to CONTENT_SETTING_BLOCK
  content_settings_info_.erase(CONTENT_SETTINGS_TYPE_PLUGINS);
  website_settings_registry_->UnRegister(CONTENT_SETTINGS_TYPE_PLUGINS);
  Register(
      CONTENT_SETTINGS_TYPE_PLUGINS, "plugins",
      CONTENT_SETTING_BLOCK, WebsiteSettingsInfo::SYNCABLE,
      WhitelistedSchemes(kChromeUIScheme, kChromeDevToolsScheme),
      ValidSettings(CONTENT_SETTING_ALLOW, CONTENT_SETTING_BLOCK,
                    CONTENT_SETTING_ASK,
                    CONTENT_SETTING_DETECT_IMPORTANT_CONTENT),
      WebsiteSettingsInfo::SINGLE_ORIGIN_WITH_EMBEDDED_EXCEPTIONS_SCOPE,
      WebsiteSettingsRegistry::DESKTOP,
      ContentSettingsInfo::INHERIT_IF_LESS_PERMISSIVE,
      ContentSettingsInfo::EPHEMERAL);
}

} // namespace content_settings
