#include "ui/base/webui/web_ui_util.h"

#define SetLoadTimeDataDefaults SetLoadTimeDataDefaults_ChromiumImpl
#include "../../../../../ui/base/webui/web_ui_util.cc"
#undef SetLoadTimeDataDefaults

#include "chrome/grit/generated_resources.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace webui {

void SetLoadTimeDataDefaults(const std::string& app_locale,
                             base::DictionaryValue* localized_strings) {

  SetLoadTimeDataDefaults_ChromiumImpl(app_locale, localized_strings);
  localized_strings->SetString(
    "brToolbarSettingsTitle",
    l10n_util::GetStringUTF16(IDS_SETTINGS_SETTINGS)
  );
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