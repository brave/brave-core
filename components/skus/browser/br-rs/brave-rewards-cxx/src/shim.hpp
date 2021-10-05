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
  void shim_purge();
  void shim_set(rust::cxxbridge1::Str key, rust::cxxbridge1::Str value);
  const std::string& shim_get(rust::cxxbridge1::Str key);

  void shim_scheduleWakeup(::std::uint64_t delay_ms,
                           rust::cxxbridge1::Fn<void()> done);

  void shim_executeRequest(
      const brave_rewards::HttpRequest& req,
      rust::cxxbridge1::Fn<
          void(rust::cxxbridge1::Box<brave_rewards::HttpRoundtripContext>,
               brave_rewards::HttpResponse)> done,
      rust::cxxbridge1::Box<brave_rewards::HttpRoundtripContext> ctx);
}
