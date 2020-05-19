#define BRAVE_INITIALIZE_PROFILE                                             \
  test_url_loader_factory_.AddResponse("http://localhost:8295/v2/timestamp", \
                                       R"({"timestamp": "123456"})");        \
  test_url_loader_factory_.AddResponse(                                      \
      "http://localhost:8295/v2/auth",                                       \
      R"({"access_token": "at1", "expires_in": 3600})");                     \
  profile_sync_service->SetURLLoaderFactoryForTest(                          \
      test_url_loader_factory_.GetSafeWeakWrapper());
#include "../../../../../../../chrome/browser/sync/test/integration/sync_test.cc"
#undef BRAVE_INITIALIZE_PROFILE
