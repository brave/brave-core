#include "content/browser/screen_orientation/screen_orientation_delegate_android.h"

class LockableScreenOrientationDelegate : public content::ScreenOrientationDelegateAndroid {
 public:
  LockableScreenOrientationDelegate();
  ~LockableScreenOrientationDelegate() override;

  void Lock(content::WebContents* web_contents,
            device::mojom::ScreenOrientationLockType lock_orientation) override;

  void Unlock(content::WebContents* web_contents) override;
};
