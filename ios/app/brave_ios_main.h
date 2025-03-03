#include <memory>

#include "brave/ios/app/brave_main_delegate.h"

namespace base {
class TimeTicks;
}

namespace web {
class WebMain;
}

// Encapsulates any setup and initialization that is needed by common
// Chrome code.  A single instance of this object should be created during app
// startup (or shortly after launch), and clients must ensure that this object
// is not destroyed while Chrome code is still on the stack.
class BraveIOSMain {
 public:
  BraveIOSMain();
  ~BraveIOSMain();

  // The time main() starts.  Only call from main().
  static void InitStartTime();

  // Returns the time that main() started.  Used for performance tests.
  // InitStartTime() must has been called before.
  static const base::TimeTicks& StartTime();

 private:
  BraveMainDelegate main_delegate_;
  std::unique_ptr<web::WebMain> web_main_;
};
