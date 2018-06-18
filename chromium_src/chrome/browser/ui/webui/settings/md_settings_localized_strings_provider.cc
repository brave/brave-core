#define AddCommonStrings AddCommonStrings_ChromeImpl
#include "../../../../../../chrome/browser/ui/webui/settings/md_settings_localized_strings_provider.cc"
#undef AddCommonStrings

namespace settings {

void AddCommonStrings(content::WebUIDataSource* html_source, Profile* profile) {
  AddCommonStrings_ChromeImpl(html_source, profile);
  LocalizedString localized_strings[] = {
    {"siteSettingsAutoplay",
      IDS_SETTINGS_SITE_SETTINGS_AUTOPLAY},
    {"siteSettingsCategoryAutoplay",
      IDS_SETTINGS_SITE_SETTINGS_AUTOPLAY},
    {"siteSettingsAutoplayAsk",
      IDS_SETTINGS_SITE_SETTINGS_AUTOPLAY_ASK},
    {"siteSettingsAutoplayAskRecommended",
      IDS_SETTINGS_SITE_SETTINGS_AUTOPLAY_ASK_RECOMMENDED}
  };
  AddLocalizedStringsBulk(html_source, localized_strings,
                          arraysize(localized_strings));
}

}
