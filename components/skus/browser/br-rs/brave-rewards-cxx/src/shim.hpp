#include <memory>
#include <functional>
#include "cxx.h"

#include "brave/components/skus/common/skus_sdk.mojom.h"

namespace brave_rewards {
  enum class RewardsResult: uint8_t;
  struct HttpRequest;
  struct HttpResponse;
  struct HttpRoundtripContext;

  class RefreshOrderCallbackState {
   public:
    skus::mojom::SkusSdk::RefreshOrderCallback cb;
    //TODO: possibly store Prefs object here
  };

  using RefreshOrderCallback = void (*) (
      RefreshOrderCallbackState* callback_state,
      RewardsResult result,
      rust::cxxbridge1::Str order
  );

  void shim_executeRequest(const brave_rewards::HttpRequest&, rust::cxxbridge1::Fn<void(rust::cxxbridge1::Box<brave_rewards::HttpRoundtripContext>, brave_rewards::HttpResponse)>, rust::cxxbridge1::Box<brave_rewards::HttpRoundtripContext>);
}
