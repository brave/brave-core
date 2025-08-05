#include "lockable_screen_orientation_delegate.h"
#include "content/browser/screen_orientation/screen_orientation_provider.h"

LockableScreenOrientationDelegate::LockableScreenOrientationDelegate() {
    content::ScreenOrientationProvider::SetDelegate(this);
}
LockableScreenOrientationDelegate::~LockableScreenOrientationDelegate() {
  content::ScreenOrientationProvider::SetDelegate(new content::ScreenOrientationDelegateAndroid());
}

void LockableScreenOrientationDelegate::Lock(
    content::WebContents* web_contents,
    device::mojom::ScreenOrientationLockType lock_orientation) {
  // No-op.
}

void LockableScreenOrientationDelegate::Unlock(content::WebContents* web_contents) {
  // No-op.
}

