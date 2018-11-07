#include "components/spellcheck/common/spellcheck_common.h"

#include <vector>
#include "base/files/file_path.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
struct LanguageFilename {
  const char* language;  // The language input.
  const char* filename;  // The corresponding filename.
};
}

class SpellcheckTest : public testing::Test {
};

TEST_F(SpellcheckTest, NumberOfLanguages) {
  // Check that there are exactly as many supported spellcheck languages
  // as we expect. Failure of this test means we need to add another
  // dictionary to the download server. Check the kSupportedSpellCheckerLanguages
  // array in components/spellcheck/spellcheck_common.cc to find the new
  // language.
  std::vector<std::string> languages = spellcheck::SpellCheckLanguages();
  EXPECT_EQ(languages.size(), 48UL);
}

TEST_F(SpellcheckTest, DictionaryFilenames) {
  // Check the filename of the dictionary for each supported spellcheck
  // language. Failure of this test means a dictionary has been updated
  // with a new version and the filename has changed. The test failure log
  // should show the expected filename, or check GetVersionedFilename() in
  // components/spellcheck/spellcheck_common.cc to determine the new filename,
  // then add the updated dictionary file to the download server.
  static constexpr LanguageFilename kExpectedSpellCheckerFilenames[] = {
    {"af", "af-ZA-3-0.bdic"},
    {"bg", "bg-BG-3-0.bdic"},
    {"ca", "ca-ES-3-0.bdic"},
    {"cs", "cs-CZ-3-0.bdic"},
    {"da", "da-DK-3-0.bdic"},
    {"de", "de-DE-3-0.bdic"},
    {"el", "el-GR-3-0.bdic"},
    {"en-AU", "en-AU-8-0.bdic"},
    {"en-CA", "en-CA-8-0.bdic"},
    {"en-GB", "en-GB-8-0.bdic"},
    {"en-US", "en-US-8-0.bdic"},
    {"es", "es-ES-3-0.bdic"},
    {"es-419", "es-ES-3-0.bdic"},
    {"es-AR", "es-ES-3-0.bdic"},
    {"es-ES", "es-ES-3-0.bdic"},
    {"es-MX", "es-ES-3-0.bdic"},
    {"es-US", "es-ES-3-0.bdic"},
    {"et", "et-EE-3-0.bdic"},
    {"fa", "fa-IR-7-0.bdic"},
    {"fo", "fo-FO-3-0.bdic"},
    {"fr", "fr-FR-3-0.bdic"},
    {"he", "he-IL-3-0.bdic"},
    {"hi", "hi-IN-3-0.bdic"},
    {"hr", "hr-HR-3-0.bdic"},
    {"hu", "hu-HU-3-0.bdic"},
    {"id", "id-ID-3-0.bdic"},
    {"it", "it-IT-3-0.bdic"},
    {"ko", "ko-3-0.bdic"},
    {"lt", "lt-LT-3-0.bdic"},
    {"lv", "lv-LV-3-0.bdic"},
    {"nb", "nb-NO-3-0.bdic"},
    {"nl", "nl-NL-3-0.bdic"},
    {"pl", "pl-PL-3-0.bdic"},
    {"pt-BR", "pt-BR-3-0.bdic"},
    {"pt-PT", "pt-PT-3-0.bdic"},
    {"ro", "ro-RO-3-0.bdic"},
    {"ru", "ru-RU-3-0.bdic"},
    {"sh", "sh-3-0.bdic"},
    {"sk", "sk-SK-3-0.bdic"},
    {"sl", "sl-SI-3-0.bdic"},
    {"sq", "sq-3-0.bdic"},
    {"sr", "sr-3-0.bdic"},
    {"sv", "sv-SE-3-0.bdic"},
    {"ta", "ta-IN-3-0.bdic"},
    {"tg", "tg-TG-5-0.bdic"},
    {"tr", "tr-TR-4-0.bdic"},
    {"uk", "uk-UA-3-0.bdic"},
    {"vi", "vi-VN-3-0.bdic"}
  };
  for (const auto& lang_file : kExpectedSpellCheckerFilenames) {
      base::FilePath dict_dir = base::FilePath(lang_file.filename);
      EXPECT_EQ(spellcheck::GetVersionedFileName(
                    lang_file.language, base::FilePath()), dict_dir);
  }
};
