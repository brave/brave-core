//
//  BraveBrowserStateIOData.hpp
//  Sources
//
//  Created by brandon on 2020-05-06.
//

#ifndef BraveBrowserStateIOData_h
#define BraveBrowserStateIOData_h

#include "brave/vendor/brave-ios/components/browser_state/chrome_browser_state.h"

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "components/prefs/pref_store.h"

namespace content_settings {
class CookieSettings;
}

namespace net {
class CookieStore;
class HttpServerProperties;
class HttpTransactionFactory;
class ProxyConfigService;
class ProxyResolutionService;
class ReportSender;
class SystemCookieStore;
class TransportSecurityPersister;
class TransportSecurityState;
class URLRequestJobFactoryImpl;
}  // namespace net

namespace brave {
class BraveBrowserStateIOData {
 public:
  virtual ~BraveBrowserStateIOData();
    
  //void Init(ProtocolHandlerMap* protocol_handlers) const;

  ChromeBrowserStateType browser_state_type() const {
    return browser_state_type_;
  }

  bool IsOffTheRecord() const;
  void InitializeMetricsEnabledStateOnUIThread();
  bool GetMetricsEnabledStateOnIOThread() const;
  const ChromeBrowserStateType browser_state_type_;
    
  class Handle {
   public:
    explicit Handle(ChromeBrowserState* browser_state);
    ~Handle();
      
    void Init(const base::FilePath& cookie_path,
              const base::FilePath& cache_path,
              int cache_max_size,
              const base::FilePath& profile_path);

    BraveBrowserStateIOData* io_data() const;

   private:
    void LazyInitialize() const;

    BraveBrowserStateIOData* const io_data_;
    ChromeBrowserState* const browser_state_;
    mutable bool initialized_;

    DISALLOW_COPY_AND_ASSIGN(Handle);
  };
private:
  DISALLOW_COPY_AND_ASSIGN(BraveBrowserStateIOData);
};
}

#endif /* BraveBrowserStateIOData_h */
