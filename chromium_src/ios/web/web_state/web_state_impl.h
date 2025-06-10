#include "base/observer_list.h"

namespace web {
class WebUIIOS;
}  // namespace web

#define observers_ \
  observers_;      \
  std::vector<std::unique_ptr<web::WebUIIOS>> brave_web_uis_

#define CancelDialogs                                          \
  CancelDialogs();                                             \
  void TearDownBraveWebUI();                                   \
  void CreateBraveWebUI(const GURL& url);                      \
  bool HasBraveWebUI() const;                                  \
  void HandleBraveWebUIMessage(const GURL& source_url,         \
                               std::string_view message,       \
                               const base::Value::List& args); \
  web::WebUIIOS* GetMainWebUI() const;                         \
  void ClearBraveWebUI

#include "src/ios/web/web_state/web_state_impl.h"

#undef CancelDialogs

#undef observers_
