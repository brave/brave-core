#define HasOAuthClientConfigured HasOAuthClientConfigured_ChromiumImpl
#include "../../../../google_apis/google_api_keys.cc"
#undef HasOAuthClientConfigured

namespace google_apis {

bool HasOAuthClientConfigured() {
  // While safe browsing & geo location are working correctly, skip checking
  // if OAuthClient is configured to avoid 'Missing API Keys' message bar.
  return true;
}

}  // namespace google_apis
