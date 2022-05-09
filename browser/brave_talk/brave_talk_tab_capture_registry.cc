#include "brave/browser/brave_talk/brave_talk_tab_capture_registry.h"
#include <memory>
#include "base/lazy_instance.h"
#include "chrome\common\extensions\api\tab_capture.h"
#include "components\sessions\content\session_tab_helper.h"
#include "content/public/browser/render_process_host.h"
#include "content\public\browser\web_contents_observer.h"
#include "content\public\browser\desktop_streams_registry.h"


using extensions::api::tab_capture::TabCaptureState;
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
      registry_->KillRequest(this); // Deletes |this|.
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

std::string BraveTalkTabCaptureRegistry::AddRequest(
    content::WebContents* target_contents,
    const GURL& origin,
    content::DesktopMediaID source,
    content::WebContents* caller_contents) {
  std::string device_id;
  LiveRequest* const request = FindRequest(target_contents);

  if (request) {
      if (request->capture_state() == TabCaptureState::TAB_CAPTURE_STATE_PENDING || request->capture_state() == TabCaptureState::TAB_CAPTURE_STATE_ACTIVE) {
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
          content::kRegistryStreamTypeTab
      );
  }

  LOG(ERROR) << "Made device id: " << device_id;
  return device_id;
}

void BraveTalkTabCaptureRegistry::DispatchStatusChangeEvent(
    const LiveRequest* request) const {
  // TODO: If we have a streamId, dispatch it to the content process.
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

}  // namespace brave_talk