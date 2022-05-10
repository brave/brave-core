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
        capture_state_(TabCaptureState::TAB_CAPTURE_STATE_NONE),
        is_verified_(false),
        is_fullscreened_(false),
        render_process_id_(
            target_contents->GetMainFrame()->GetProcess()->GetID()),
        render_frame_id_(target_contents->GetMainFrame()->GetRoutingID()) {
    DCHECK(web_contents());
    DCHECK(registry_);
  }

  LiveRequest(const LiveRequest&) = delete;
  LiveRequest& operator=(const LiveRequest&) = delete;

  ~LiveRequest() override {}

  TabCaptureState capture_state() const { return capture_state_; }
  bool is_verified() const { return is_verified_; }
  void SetIsVerified() {
    DCHECK(!is_verified_);
    is_verified_ = true;
  }

  bool WasTargettingRenderFrameID(int render_process_id,
                                  int render_frame_id) const {
    return render_process_id_ == render_process_id &&
           render_frame_id_ == render_frame_id;
  }

  void UpdateCaptureState(TabCaptureState next_capture_state) {
    if (capture_state_ == next_capture_state)
      return;

    capture_state_ = next_capture_state;
    registry_->DispatchStatusChangeEvent(this);
  }

  void GetCaptureInfo(extensions::api::tab_capture::CaptureInfo* info) {
    info->tab_id = sessions::SessionTabHelper::IdForTab(web_contents()).id();
    info->status = capture_state_;
    info->fullscreen = is_fullscreened_;
  }

 protected:
  void DidToggleFullscreenModeForTab(bool entered_fullscreen,
                                     bool will_cause_resize) override {
    is_fullscreened_ = entered_fullscreen;
    if (capture_state_ == TabCaptureState::TAB_CAPTURE_STATE_ACTIVE)
      registry_->DispatchStatusChangeEvent(this);
  }

  void WebContentsDestroyed() override {
    registry_->KillRequest(this);  // Deletes |this|.
  }

 private:
  const raw_ptr<BraveTalkTabCaptureRegistry> registry_;
  TabCaptureState capture_state_;
  bool is_verified_;
  bool is_fullscreened_;

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
    if (request->capture_state() ==
            TabCaptureState::TAB_CAPTURE_STATE_PENDING ||
        request->capture_state() == TabCaptureState::TAB_CAPTURE_STATE_ACTIVE) {
      return device_id;
    } else {
      // Delete the request before creating it's replacement.
      KillRequest(request);
    }
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

void BraveTalkTabCaptureRegistry::OnRequestUpdate(
    int render_process_id,
    int render_frame_id,
    blink::mojom::MediaStreamType stream_type,
    const content::MediaRequestState new_state) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (stream_type != blink::mojom::MediaStreamType::DISPLAY_AUDIO_CAPTURE &&
      stream_type != blink::mojom::MediaStreamType::DISPLAY_VIDEO_CAPTURE) {
    return;
  }
  auto* request = FindRequest(render_process_id, render_frame_id);
  if (!request)
    return;

  TabCaptureState next_state =
      extensions::api::tab_capture::TAB_CAPTURE_STATE_NONE;
  switch (new_state) {
    case content::MEDIA_REQUEST_STATE_PENDING_APPROVAL:
      next_state = tab_capture::TAB_CAPTURE_STATE_PENDING;
      break;
    case content::MEDIA_REQUEST_STATE_DONE:
      next_state = tab_capture::TAB_CAPTURE_STATE_ACTIVE;
      break;
    case content::MEDIA_REQUEST_STATE_CLOSING:
      next_state = tab_capture::TAB_CAPTURE_STATE_STOPPED;
      break;
    case content::MEDIA_REQUEST_STATE_ERROR:
      next_state = tab_capture::TAB_CAPTURE_STATE_ERROR;
      break;
    case content::MEDIA_REQUEST_STATE_OPENING:
      return;
    case content::MEDIA_REQUEST_STATE_REQUESTED:
    case content::MEDIA_REQUEST_STATE_NOT_REQUESTED:
      NOTREACHED();
      return;
  }

  if (next_state == tab_capture::TAB_CAPTURE_STATE_PENDING
    && request->capture_state() != tab_capture::TAB_CAPTURE_STATE_PENDING
    && request->capture_state() != tab_capture::TAB_CAPTURE_STATE_NONE
    && request->capture_state() != tab_capture::TAB_CAPTURE_STATE_STOPPED) {
      NOTREACHED() << "Trying to capture tab with existing capture";
      return;
    }

    request->UpdateCaptureState(next_state);
}

void BraveTalkTabCaptureRegistry::DispatchStatusChangeEvent(
    const LiveRequest* request) const {
  // TODO: If we have a streamId, dispatch it to the content process.
  // NOt this.

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

  if (request->is_verified() ||
      (request->capture_state() !=
           extensions::api::tab_capture::TAB_CAPTURE_STATE_NONE &&
       request->capture_state() !=
           extensions::api::tab_capture::TAB_CAPTURE_STATE_PENDING))
    return false;

  request->SetIsVerified();
  return true;
}

}  // namespace brave_talk