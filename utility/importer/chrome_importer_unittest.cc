#include "brave/utility/importer/chrome_importer.h"
#include "brave/common/brave_paths.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "chrome/common/chrome_paths.h"
#include "testing/gtest/include/gtest/gtest.h"

// In order to test the Chrome import functionality effectively, we store a
// simulated Library directory containing dummy data files in the same
// structure as ~/Library in the Brave test data directory.
// This function returns the path to that directory.
base::FilePath GetTestChromeLibraryPath(const std::string& suffix) {
  base::FilePath test_dir;
  PathService::Get(brave::DIR_TEST_DATA, &test_dir);

  // Our simulated ~/Library directory
  return
      test_dir.AppendASCII("import").AppendASCII("chrome").AppendASCII(suffix);
}

TEST(ChromeImporterTest, TestTruth) {
  brave::RegisterPathProvider();
  base::FilePath test_library_dir = GetTestChromeLibraryPath("default");
  CHECK(base::PathExists(test_library_dir));
}
