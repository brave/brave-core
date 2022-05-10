#include "brave/browser/brave_talk/brave_talk_tab_capture_registry.h"
#include <memory>
#include "base/lazy_instance.h"
#include "chrome\common\extensions\api\tab_capture.h"
#include "components\sessions\content\session_tab_helper.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "content\public\browser\desktop_streams_registry.h"
#include "content\public\browser\web_contents_observer.h"

namespace tab_capture = extensions::api::tab_capture;
using tab_capture::TabCaptureState;

namespace brave_talk {

class BraveTalkTabCaptureRegistry::LiveRequest
    : public content::WebContentsObserver {
 public:
  LiveRequest(content::WebContents* target_contents,
              BraveTalkTabCaptureRegistry* registry)
      : content::WebContentsObserver(target_contents),
        registry_(registry),
        render_process_id_(
            target_contents->GetMainFrame()->GetProcess()->GetID()),
        render_frame_id_(target_contents->GetMainFrame()->GetRoutingID()) {
    DCHECK(web_contents());
    DCHECK(registry_);
  }

  LiveRequest(const LiveRequest&) = delete;
  LiveRequest& operator=(const LiveRequest&) = delete;

  ~LiveRequest() override {}

  bool WasTargettingRenderFrameID(int render_process_id,
                                  int render_frame_id) const {
    return render_process_id_ == render_process_id &&
           render_frame_id_ == render_frame_id;
  }

 protected:
  void WebContentsDestroyed() override {
    registry_->KillRequest(this);  // Deletes |this|.
  }

 private:
  const raw_ptr<BraveTalkTabCaptureRegistry> registry_;

  int render_process_id_;
  int render_frame_id_;
};

BraveTalkTabCaptureRegistry::BraveTalkTabCaptureRegistry(
    content::BrowserContext* context)
    : browser_context_(context) {
  MediaCaptureDevicesDispatcher::GetInstance()->AddObserver(this);
}

BraveTalkTabCaptureRegistry::~BraveTalkTabCaptureRegistry() {
  MediaCaptureDevicesDispatcher::GetInstance()->RemoveObserver(this);
}

BraveTalkTabCaptureRegistry* BraveTalkTabCaptureRegistry::Get(
    content::BrowserContext* context) {
  return extensions::BrowserContextKeyedAPIFactory<
      BraveTalkTabCaptureRegistry>::Get(context);
}

static base::LazyInstance<extensions::BrowserContextKeyedAPIFactory<
    BraveTalkTabCaptureRegistry>>::DestructorAtExit
    g_brave_talk_tab_capture_registry_factory = LAZY_INSTANCE_INITIALIZER;
// static
extensions::BrowserContextKeyedAPIFactory<BraveTalkTabCaptureRegistry>*
BraveTalkTabCaptureRegistry::GetFactoryInstance() {
  return g_brave_talk_tab_capture_registry_factory.Pointer();
}

std::string BraveTalkTabCaptureRegistry::AddRequest(
    content::WebContents* target_contents,
    const GURL& origin,
    content::DesktopMediaID source,
    content::WebContents* caller_contents) {
  std::string device_id;
  LiveRequest* const request = FindRequest(target_contents);

  if (request) {
    // Delete the request before creating it's replacement.
    KillRequest(request);
  }

  requests_.push_back(std::make_unique<LiveRequest>(target_contents, this));
  auto* const main_frame = caller_contents->GetMainFrame();
  if (main_frame) {
    device_id = content::DesktopStreamsRegistry::GetInstance()->RegisterStream(
        main_frame->GetProcess()->GetID(), main_frame->GetRoutingID(),
        url::Origin::Create(origin), source, "brave_talk",
        content::kRegistryStreamTypeTab);
  }

  LOG(ERROR) << "Made device id: " << device_id;
  return device_id;
}

BraveTalkTabCaptureRegistry::LiveRequest*
BraveTalkTabCaptureRegistry::FindRequest(
    const content::WebContents* contents) const {
  for (const auto& request : requests_) {
    if (request->web_contents() == contents)
      return request.get();
  }
  return nullptr;
}

BraveTalkTabCaptureRegistry::LiveRequest*
BraveTalkTabCaptureRegistry::FindRequest(int target_render_process_id,
                                         int target_render_frame_id) const {
  for (const auto& request : requests_) {
    if (request->WasTargettingRenderFrameID(target_render_process_id,
                                            target_render_frame_id))
      return request.get();
  }
  return nullptr;
}

void BraveTalkTabCaptureRegistry::KillRequest(LiveRequest* request) {
  for (auto it = requests_.begin(); it != requests_.end(); ++it) {
    if (it->get() == request) {
      requests_.erase(it);
      return;
    }
  }
  NOTREACHED();
}

bool BraveTalkTabCaptureRegistry::VerifyRequest(int target_render_process_id,
                                                int target_render_frame_id,
                                                int source_render_process_id,
                                                int source_render_frame_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  LiveRequest* const request =
      FindRequest(target_render_frame_id, target_render_frame_id);
  if (!request) {
    return false;  // Unknown RenderFrameHost ID, or frame has gone away.
  }

  // TODO: Workout what to do here.
  return true;
}

}  // namespace brave_talk