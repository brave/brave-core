#ifndef BRAVE_BRAVE_BROWSER_SHARING_HUB_BRAVE_SHARING_HUB_SERVICE_H_
#define BRAVE_BRAVE_BROWSER_SHARING_HUB_BRAVE_SHARING_HUB_SERVICE_H_

#include <vector>
#include "chrome/browser/sharing_hub/sharing_hub_model.h"

namespace content {
class BrowserContext;
class WebContents;
}

namespace sharing_hub {
class BraveSharingHubModel : public sharing_hub::SharingHubModel {
 public:
  explicit BraveSharingHubModel(content::BrowserContext* context);
  BraveSharingHubModel(const SharingHubModel&) = delete;
  BraveSharingHubModel& operator=(const SharingHubModel&) = delete;
  ~BraveSharingHubModel() override;

private:
  SharingHubAction brave_talk_share_tab_action_;
  
  void GetFirstPartyActionList(content::WebContents* web_contents,
                               std::vector<SharingHubAction>* list) override;
};
}  // namespace sharing_hub

#endif  // BRAVE_BRAVE_BROWSER_SHARING_HUB_BRAVE_SHARING_HUB_SERVICE_H_
