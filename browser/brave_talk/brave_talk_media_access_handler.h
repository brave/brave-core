#ifndef BRAVE_BROWSER_BRAVE_TALK_BRAVE_TALK_MEDIA_ACCESS_HANDLER_H_
#define BRAVE_BROWSER_BRAVE_TALK_BRAVE_TALK_MEDIA_ACCESS_HANDLER_H_

#include "chrome\browser\media\capture_access_handler_base.h"

namespace brave_talk {

class BraveTalkMediaAccessHandler : public CaptureAccessHandlerBase {
 public:
  BraveTalkMediaAccessHandler();

  BraveTalkMediaAccessHandler(const BraveTalkMediaAccessHandler&) = delete;
  BraveTalkMediaAccessHandler& operator=(const BraveTalkMediaAccessHandler&) =
      delete;

  ~BraveTalkMediaAccessHandler() override;
};

}  // namespace brave_talk

#endif