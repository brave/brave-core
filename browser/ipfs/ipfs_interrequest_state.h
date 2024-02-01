
#include "base/supports_user_data.h"

namespace content {
class BrowserContext;
}

namespace ipfs {

class InterRequestState : public base::SupportsUserData::Data {

public:
  InterRequestState();
  ~InterRequestState() override;

  static InterRequestState* FromBrowserContext(content::BrowserContext*);
};

}  // namespace ipfs    