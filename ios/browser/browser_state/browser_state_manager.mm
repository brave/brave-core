#include "brave/ios/browser/browser_state/browser_state_manager.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/mac/foundation_util.h"
#include "base/sequenced_task_runner.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"
#include "base/threading/thread_restrictions.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state_impl.h"
#include "ios/chrome/browser/chrome_constants.h"

namespace {

const base::FilePath::CharType kProductDirName[] = FILE_PATH_LITERAL("Brave");

bool GetDefaultUserDataDirectory(base::FilePath* result) {
  if (!base::mac::GetUserDirectory(NSApplicationSupportDirectory,
                                                 result)) {
    NOTREACHED();
    return false;
  }

  if (!base::PathExists(*result) && !base::CreateDirectory(*result))
    return false;

  *result = result->Append(kProductDirName);
  return true;
}

base::FilePath GetUserDataDir() {
  base::FilePath user_data_dir;
  bool result = GetDefaultUserDataDirectory(&user_data_dir);
  DCHECK(result);
  return user_data_dir;
}

}  // namespace

BrowserStateManager& BrowserStateManager::GetInstance() {
  static BrowserStateManager instance;
  return instance;
}

BrowserStateManager::BrowserStateManager() : browser_state_(nullptr) {}

BrowserStateManager::~BrowserStateManager() {}

ChromeBrowserState* BrowserStateManager::GetBrowserState() {
  if (!browser_state_) {
    // Get sequenced task runner for making sure that file operations of
    // this profile are executed in expected order (what was previously assured by
    // the FILE thread).
    scoped_refptr<base::SequencedTaskRunner> io_task_runner =
        base::ThreadPool::CreateSequencedTaskRunner(
            {base::TaskShutdownBehavior::BLOCK_SHUTDOWN, base::MayBlock()});
    browser_state_ = std::make_unique<ChromeBrowserStateImpl>(
        io_task_runner,
        GetUserDataDir().Append(kIOSChromeInitialBrowserState));
  }
  CHECK(browser_state_.get());
  return browser_state_.get();
}
