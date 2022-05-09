#ifndef BRAVE_BROWSER_BRAVE_TALK_BRAVE_TALK_MEDIA_ACCESS_HANDLER_H_
#define BRAVE_BROWSER_BRAVE_TALK_BRAVE_TALK_MEDIA_ACCESS_HANDLER_H_

#include "chrome\browser\media\capture_access_handler_base.h"
#include "content\public\browser\media_stream_request.F"

namespace contents {
class WebContents;
}

namespace extensions {
class Extensions;
}
namespace brave_talk {

class BraveTalkMediaAccessHandler : public CaptureAccessHandlerBase {
 public:
  BraveTalkMediaAccessHandler();

  BraveTalkMediaAccessHandler(const BraveTalkMediaAccessHandler&) = delete;
  BraveTalkMediaAccessHandler& operator=(const BraveTalkMediaAccessHandler&) =
      delete;
  ~BraveTalkMediaAccessHandler() override;

  void HandleRequest(content::WebContents* web_contents,
                     const content::MediaStreamRequest& request,
                     content::MediaResponseCallback callback,
                     const extensions::Extension* extension) override;
};

}  // namespace brave_talk

#endif