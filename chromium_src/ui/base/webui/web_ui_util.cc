#include "ui/base/webui/web_ui_util.h"

#include "brave/ui/webui/resources/grit/brave_webui_resources.h"
#include "ui/resources/grit/webui_resources.h"

#define SetLoadTimeDataDefaults SetLoadTimeDataDefaults_ChromiumImpl
#define IDR_WEBUI_CSS_TEXT_DEFAULTS_MD_PREVIOUS IDR_WEBUI_CSS_TEXT_DEFAULTS_MD
#undef IDR_WEBUI_CSS_TEXT_DEFAULTS_MD
#define IDR_WEBUI_CSS_TEXT_DEFAULTS_MD IDR_BRAVE_WEBUI_CSS_TEXT_DEFAULTS
#include "../../../../../ui/base/webui/web_ui_util.cc"
#undef SetLoadTimeDataDefaults
#undef IDR_WEBUI_CSS_TEXT_DEFAULTS_MD
#define IDR_WEBUI_CSS_TEXT_DEFAULTS_MD IDR_WEBUI_CSS_TEXT_DEFAULTS_MD_PREVIOUS

#include "chrome/grit/generated_resources.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace webui {

void SetLoadTimeDataDefaults(const std::string& app_locale,
                             base::DictionaryValue* localized_strings) {

  SetLoadTimeDataDefaults_ChromiumImpl(app_locale, localized_strings);
#if !defined(OS_ANDROID)
  localized_strings->SetString(
    "brToolbarSettingsTitle",
    l10n_util::GetStringUTF16(IDS_SETTINGS_SETTINGS)
  );
#endif
  localized_strings->SetString(
    "brToolbarBookmarksTitle",
    l10n_util::GetStringUTF16(IDS_BOOKMARK_MANAGER_TITLE)
  );
  localized_strings->SetString(
    "brToolbarDownloadsTitle",
    l10n_util::GetStringUTF16(IDS_DOWNLOAD_TITLE)
  );
  localized_strings->SetString(
    "brToolbarHistoryTitle",
    l10n_util::GetStringUTF16(IDS_HISTORY_TITLE)
  );
}

void SetLoadTimeDataDefaults(const std::string& app_locale,
                             ui::TemplateReplacements* replacements) {
  SetLoadTimeDataDefaults_ChromiumImpl(app_locale, replacements);
}

}
