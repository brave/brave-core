#ifndef BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_RENDERER_HOST_ANCESTOR_THROTTLE_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_RENDERER_HOST_ANCESTOR_THROTTLE_H_

#include "base/functional/callback.h"
#include "url/origin.h"

namespace content {
class BrowserContext;
}

// Add injection point before the private section
#define EvaluateEmbeddingOptIn(...) unused_var; \
 public: \
  using PermissionCallback = base::RepeatingCallback<bool(content::BrowserContext*, const url::Origin&)>; \
  static void SetPermissionCallback(PermissionCallback callback); \
 private: \
  static bool CheckPermissionForOrigin(content::BrowserContext* browser_context, const url::Origin& origin); \
  NavigationThrottle::ThrottleCheckResult WillProcessResponse_ChromiumImpl(); \
  static PermissionCallback* permission_callback_; \
  CheckResult EvaluateEmbeddingOptIn(LoggingDisposition logging)

#include <content/browser/renderer_host/ancestor_throttle.h>  // IWYU pragma: export

#undef EvaluateEmbeddingOptIn

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_RENDERER_HOST_ANCESTOR_THROTTLE_H_