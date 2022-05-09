#include "brave/browser/brave_talk/brave_talk_media_access_handler.h"

namespace brave_talk {

BraveTalkMediaAccessHandler::BraveTalkMediaAccessHandler() {}
BraveTalkMediaAccessHandler::~BraveTalkMediaAccessHandler() = default;

void BraveTalkMediaAccessHandler::HandleRequest(
    content::WebContents* web_contents,
    const content::MediaStreamRequest& request,
    content::MediaResponseCallback callback,
    const extensions::Extension* extension) {
  LOG(ERROR) << "WOW. Tried to handle a request :O";
}

}  // namespace brave_talk