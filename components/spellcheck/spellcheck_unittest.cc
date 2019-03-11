#include "components/spellcheck/common/spellcheck_common.h"

#include <vector>
#include "base/files/file_path.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
struct LanguageFilename {
  const char* language;  // The language input.
  base::FilePath::StringType filename;  // The corresponding filename.
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
  LanguageFilename kExpectedSpellCheckerFilenames[] = {
    {"af", FILE_PATH_LITERAL("af-ZA-3-0.bdic")},
    {"bg", FILE_PATH_LITERAL("bg-BG-3-0.bdic")},
    {"ca", FILE_PATH_LITERAL("ca-ES-3-0.bdic")},
    {"cs", FILE_PATH_LITERAL("cs-CZ-3-0.bdic")},
    {"da", FILE_PATH_LITERAL("da-DK-3-0.bdic")},
    {"de", FILE_PATH_LITERAL("de-DE-3-0.bdic")},
    {"el", FILE_PATH_LITERAL("el-GR-3-0.bdic")},
    {"en-AU", FILE_PATH_LITERAL("en-AU-8-0.bdic")},
    {"en-CA", FILE_PATH_LITERAL("en-CA-8-0.bdic")},
    {"en-GB", FILE_PATH_LITERAL("en-GB-8-0.bdic")},
    {"en-US", FILE_PATH_LITERAL("en-US-8-0.bdic")},
    {"es", FILE_PATH_LITERAL("es-ES-3-0.bdic")},
    {"es-419", FILE_PATH_LITERAL("es-ES-3-0.bdic")},
    {"es-AR", FILE_PATH_LITERAL("es-ES-3-0.bdic")},
    {"es-ES", FILE_PATH_LITERAL("es-ES-3-0.bdic")},
    {"es-MX", FILE_PATH_LITERAL("es-ES-3-0.bdic")},
    {"es-US", FILE_PATH_LITERAL("es-ES-3-0.bdic")},
    {"et", FILE_PATH_LITERAL("et-EE-3-0.bdic")},
    {"fa", FILE_PATH_LITERAL("fa-IR-7-0.bdic")},
    {"fo", FILE_PATH_LITERAL("fo-FO-3-0.bdic")},
    {"fr", FILE_PATH_LITERAL("fr-FR-3-0.bdic")},
    {"he", FILE_PATH_LITERAL("he-IL-3-0.bdic")},
    {"hi", FILE_PATH_LITERAL("hi-IN-3-0.bdic")},
    {"hr", FILE_PATH_LITERAL("hr-HR-3-0.bdic")},
    {"hu", FILE_PATH_LITERAL("hu-HU-3-0.bdic")},
    {"id", FILE_PATH_LITERAL("id-ID-3-0.bdic")},
    {"it", FILE_PATH_LITERAL("it-IT-3-0.bdic")},
    {"ko", FILE_PATH_LITERAL("ko-3-0.bdic")},
    {"lt", FILE_PATH_LITERAL("lt-LT-3-0.bdic")},
    {"lv", FILE_PATH_LITERAL("lv-LV-3-0.bdic")},
    {"nb", FILE_PATH_LITERAL("nb-NO-3-0.bdic")},
    {"nl", FILE_PATH_LITERAL("nl-NL-3-0.bdic")},
    {"pl", FILE_PATH_LITERAL("pl-PL-3-0.bdic")},
    {"pt-BR", FILE_PATH_LITERAL("pt-BR-3-0.bdic")},
    {"pt-PT", FILE_PATH_LITERAL("pt-PT-3-0.bdic")},
    {"ro", FILE_PATH_LITERAL("ro-RO-3-0.bdic")},
    {"ru", FILE_PATH_LITERAL("ru-RU-3-0.bdic")},
    {"sh", FILE_PATH_LITERAL("sh-3-0.bdic")},
    {"sk", FILE_PATH_LITERAL("sk-SK-3-0.bdic")},
    {"sl", FILE_PATH_LITERAL("sl-SI-3-0.bdic")},
    {"sq", FILE_PATH_LITERAL("sq-3-0.bdic")},
    {"sr", FILE_PATH_LITERAL("sr-3-0.bdic")},
    {"sv", FILE_PATH_LITERAL("sv-SE-3-0.bdic")},
    {"ta", FILE_PATH_LITERAL("ta-IN-3-0.bdic")},
    {"tg", FILE_PATH_LITERAL("tg-TG-5-0.bdic")},
    {"tr", FILE_PATH_LITERAL("tr-TR-4-0.bdic")},
    {"uk", FILE_PATH_LITERAL("uk-UA-3-0.bdic")},
    {"vi", FILE_PATH_LITERAL("vi-VN-3-0.bdic")}
  };
  for (const auto& lang_file : kExpectedSpellCheckerFilenames) {
      base::FilePath dict_dir = base::FilePath(lang_file.filename);
      EXPECT_EQ(spellcheck::GetVersionedFileName(
                    lang_file.language, base::FilePath()), dict_dir);
  }
}
