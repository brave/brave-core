#include "chrome/utility/importer/importer_creator.h"
#define CreateImporterByType CreateImporterByType_ChromiumImpl
#include "../../../../../../chrome/utility/importer/importer_creator.cc"
#undef CreateImporterByType

#include "brave/utility/importer/brave_importer.h"
#include "brave/utility/importer/chrome_importer.h"
#include "brave/utility/importer/firefox_importer.h"

namespace importer {

scoped_refptr<Importer> CreateImporterByType(ImporterType type) {
  switch (type) {
    case TYPE_FIREFOX:
      return new brave::FirefoxImporter();
    case TYPE_CHROME:
      return new ChromeImporter();
    case TYPE_BRAVE:
      return new BraveImporter();
    default:
      return CreateImporterByType_ChromiumImpl(type);
  }
}

}  // namespace importer
