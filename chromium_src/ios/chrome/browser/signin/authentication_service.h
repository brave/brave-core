#ifndef IOS_CHROME_BROWSER_SIGNIN_AUTHENTICATION_SERVICE_H_
#define IOS_CHROME_BROWSER_SIGNIN_AUTHENTICATION_SERVICE_H_

#include "base/macros.h"
#include "components/keyed_service/core/keyed_service.h"

@class ChromeIdentity;

class AuthenticationService : public KeyedService {
 public:
  AuthenticationService() {}
  ~AuthenticationService() override {}

  ChromeIdentity* GetAuthenticatedIdentity() const { return nullptr; }
 private:
  DISALLOW_COPY_AND_ASSIGN(AuthenticationService);
};

#endif  // IOS_CHROME_BROWSER_SIGNIN_AUTHENTICATION_SERVICE_H_
