#ifndef BRAVE_BROWSER_BRAVE_TALK_BRAVE_TALK_SERVICE_H_
#define BRAVE_BROWSER_BRAVE_TALK_BRAVE_TALK_SERVICE_H_

#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {
class WebContents;
}

namespace share_tab_button {
class ShareTabButton;
}

namespace brave_talk {
class BraveTalkService : public KeyedService, content::WebContentsObserver {
 public:
  BraveTalkService();
  BraveTalkService(const BraveTalkService&) = delete;
  BraveTalkService& operator=(const BraveTalkService&) = delete;
  ~BraveTalkService() override;

  void GetDeviceID(content::WebContents* contents,
                   base::OnceCallback<void(std::string)> callback);

  void DidStartNavigation(content::NavigationHandle* handle) override;

  void ShareTab(content::WebContents* target_contents);

  content::WebContents* target() { return target_.get(); }

 private:
  void StartObserving(content::WebContents* contents);
  void StopObserving(content::WebContents* contents);

  base::OnceCallback<void(std::string)> on_received_device_id_;
  share_tab_button::ShareTabButton* share_tab_button();
  base::WeakPtr<content::WebContents> observing_;
  base::WeakPtr<content::WebContents> target_;
};
}  // namespace brave_talk

#endif // BRAVE_BROWSER_BRAVE_TALK_BRAVE_TALK_SERVICE_H_
