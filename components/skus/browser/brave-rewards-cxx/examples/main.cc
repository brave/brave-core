#include <iostream>
#include <vector>
#include <ctime>
#include <iomanip>
#include "wrapper.h"

using namespace std;
using namespace rust::cxxbridge1;
using namespace brave_rewards;

namespace brave_rewards {
  std::unique_ptr<brave_rewards::SkusSdkFetcher> shim_executeRequest(
      const brave_rewards::SkusSdkContext& ctx,
      const brave_rewards::HttpRequest& req, 
      rust::cxxbridge1::Fn<void(rust::cxxbridge1::Box<brave_rewards::HttpRoundtripContext>, brave_rewards::HttpResponse)> callback,
      rust::cxxbridge1::Box<brave_rewards::HttpRoundtripContext> rt_ctx
    ) {
    cout<<"url: "<<req.url<<"\n";


    std::string body = "{\"id\":\"b788a168-1136-411f-9546-43a372a2e3ed\",\"createdAt\":\"2021-08-17T21:35:48.658623Z\",\"currency\":\"USD\",\"updatedAt\":\"2021-08-17T21:36:06.174288Z\",\"totalPrice\":\"7\",\"merchantId\":\"brave.com\",\"location\":\"talk.brave.software\",\"status\":\"paid\",\"items\":[{\"id\":\"eac1b09f-2228-4f91-a970-a22b229bb994\",\"orderId\":\"b788a168-1136-411f-9546-43a372a2e3ed\",\"sku\":\"brave-talk-premium\",\"createdAt\":\"2021-08-17T21:35:48.658623Z\",\"updatedAt\":\"2021-08-17T21:35:48.658623Z\",\"currency\":\"USD\",\"quantity\":1,\"price\":\"7\",\"subtotal\":\"7\",\"location\":\"talk.brave.software\",\"description\":\"Premium access to Brave Talk\",\"credentialType\":\"time-limited\",\"validFor\":null,\"metadata\":{\"stripe_cancel_uri\":\"https://account.brave.software/plans/?intent=checkout\",\"stripe_item_id\":\"price_1J84oMHof20bphG6NBAT2vor\",\"stripe_product_id\":\"prod_Jlc224hFvAMvEp\",\"stripe_success_uri\":\"https://account.brave.software/account/?intent=provision\"}}],\"allowedPaymentMethods\":[\"stripe\"],\"metadata\":{\"stripeSubscriptionId\":\"sub_K3hLyRFkjj3mYs\"},\"lastPaidAt\":\"2021-08-17T21:36:06.174938Z\",\"expiresAt\":\"2021-09-17T08:05:09.176138Z\",\"validFor\":2629743001200000}";
    std::vector<uint8_t> body_bytes (body.begin(), body.end());

    brave_rewards::HttpResponse resp = {
      RewardsResult::Ok,
      200,
      {"foo:bar"},
      body_bytes,
    };

    callback(std::move(rt_ctx), resp);
    return nullptr;
  }

void shim_logMessage(rust::cxxbridge1::Str file,
              uint32_t line,
              TracingLevel level,
              rust::cxxbridge1::Str message) {
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::string path = std::string(file);
    std::string base_filename = path.substr(path.find_last_of("/\\") + 1);

    cout << "[";
    cout << std::put_time(&tm, "%b %d, %Y %I:%M:%S.0 %p");
    cout << ":";
    if (level == TracingLevel::Trace) {
      cout << "TRACE";
    }
    if (level == TracingLevel::Debug) {
      cout << "DEBUG";
    }
    if (level == TracingLevel::Info) {
      cout << "INFO";
    }
    if (level == TracingLevel::Warn) {
      cout << "WARN";
    }
    if (level == TracingLevel::Error) {
      cout << "ERROR";
    }
    cout<< ":" << base_filename << "(" << line << ")]" << message;
}

void shim_purge(brave_rewards::SkusSdkContext& ctx) {
    cout<<"shim_purge"<<"\n";
}

void shim_set(brave_rewards::SkusSdkContext& ctx, rust::cxxbridge1::Str key, rust::cxxbridge1::Str value) {
    cout<<"shim_set"<<"\n";
}

rust::String shim_get(brave_rewards::SkusSdkContext& ctx, rust::cxxbridge1::Str key) {
    cout<<"shim_get"<<"\n";
    //std::unique_ptr<std::string> empty(new std::string("{}"));
  //return empty;
		std::string foo = "{}";
  return foo;
}

void shim_scheduleWakeup(::std::uint64_t delay_ms,
      rust::cxxbridge1::Fn<void(rust::cxxbridge1::Box<brave_rewards::WakeupContext>)> done,
      rust::cxxbridge1::Box<brave_rewards::WakeupContext> ctx
) {
    cout<<"shim_scheduleWakeup"<<"\n";
}

}

void on_refresh_order(
  RefreshOrderCallbackState* callback_state,
  RewardsResult result,
  rust::cxxbridge1::Str order
) {
    cout<<"on_refresh_order"<<"\n";
    //callback_state->cb(result, order);
}

int main() {
  std::unique_ptr<brave_rewards::SkusSdkContext> ctx(new brave_rewards::SkusSdkContext());
  Box<CppSDK> sdk = initialize_sdk(std::move(ctx), "local");

  std::unique_ptr<RefreshOrderCallbackState> cbs (new RefreshOrderCallbackState);
  //cbs->CbFunc = [](RewardsResult result, rust::cxxbridge1::Str order) {
    //cout<<"result:"<<unsigned(result)<<"\n";
    //cout<<"order:"<<order<<"\n";
  //};

  sdk->refresh_order(on_refresh_order, std::move(cbs), "b788a168-1136-411f-9546-43a372a2e3ed");
}
