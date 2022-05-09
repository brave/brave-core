#ifndef BRAVE_BROWSER_BRAVE_TALK_BRAVE_TALK_SERVICE_H_
#define BRAVE_BROWSER_BRAVE_TALK_BRAVE_TALK_SERVICE_H_

#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {
class WebContents;
}

namespace brave_talk {
class BraveTalkService : public KeyedService, content::WebContentsObserver {
 public:
  BraveTalkService();
  BraveTalkService(const BraveTalkService&) = delete;
  BraveTalkService& operator=(const BraveTalkService&) = delete;
  ~BraveTalkService() override;

  void StartObserving(content::WebContents* contents);
  void StopObserving(content::WebContents* contents);

  void DidStartNavigation(content::NavigationHandle* handle) override;

 private:
  base::WeakPtr<content::WebContents> observing_;
};
}  // namespace brave_talk

#endif  // BRAVE_BROWSER_BRAVE_TALK_BRAVE_TALK_SERVICE_H_