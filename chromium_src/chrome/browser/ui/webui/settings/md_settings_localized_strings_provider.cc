#include "chrome/browser/ui/webui/settings/md_settings_localized_strings_provider.h"
namespace settings {
void BraveAddLocalizedStrings(content::WebUIDataSource*, Profile*);
}
#include "../../../../../../chrome/browser/ui/webui/settings/md_settings_localized_strings_provider.cc"

namespace settings {

#if !defined(OS_CHROMEOS)
void BraveAddImportDataStrings(content::WebUIDataSource* html_source) {
  LocalizedString localized_strings[] = {
    {"importCookies", IDS_SETTINGS_IMPORT_COOKIES_CHECKBOX}
  };
  AddLocalizedStringsBulk(html_source, localized_strings,
                          arraysize(localized_strings));
}
#endif

void BraveAddCommonStrings(content::WebUIDataSource* html_source, Profile* profile) {
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

void BraveAddLocalizedStrings(content::WebUIDataSource* html_source,
                              Profile* profile) {
  BraveAddImportDataStrings(html_source);
  BraveAddCommonStrings(html_source, profile);
}

}
