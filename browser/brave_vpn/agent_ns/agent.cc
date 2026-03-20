#include <curl/curl.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>

#include <optional>
#include <string>

#include "base/at_exit.h"
#include "base/check.h"
#include "base/command_line.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/logging/logging_settings.h"
#include "base/run_loop.h"
#include "base/task/single_thread_task_executor.h"
#include "base/task/thread_pool.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "mojo/core/embedder/embedder.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "net/cert/internal/trust_store_chrome.h"
#include "net/cert/x509_certificate.h"
#include "net/cert/x509_util.h"

namespace {
constexpr char kApiUrl[] =
    "https://connect-api.guardianapp.com/api/v1.3/servers/all-server-regions/"
    "city-by-country";

size_t WriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
  const size_t total = size * nmemb;
  static_cast<std::string*>(userdata)->append(ptr, total);
  return total;
}

static CURLcode SslCtxCallback(CURL* curl, void* ssl_ctx, void* /*userptr*/) {
  SSL_CTX* ctx = static_cast<SSL_CTX*>(ssl_ctx);
  X509_STORE* store = SSL_CTX_get_cert_store(ctx);

  const net::ChromeRootStoreData root_store =
      net::ChromeRootStoreData::CreateFromCompiledRootStore();

  for (const auto& anchor : root_store.trust_anchors()) {
    const auto& der = anchor.certificate->der_cert();
    const uint8_t* ptr = der.data();
    bssl::UniquePtr<X509> x509(d2i_X509(nullptr, &ptr, der.size()));
    if (x509) {
      X509_STORE_add_cert(store, x509.get());
    }
  }
  return CURLE_OK;
}

// Blocking fetch — intentionally runs off the main thread.
std::optional<std::string> FetchUrlBlocking(const char* url) {
  std::string response;
  CURL* curl = curl_easy_init();
  if (!curl) {
    LOG(ERROR) << "curl_easy_init() failed";
    return std::nullopt;
  }
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
  curl_easy_setopt(curl, CURLOPT_SSL_CTX_FUNCTION, SslCtxCallback);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);  // enforce verification
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);  // enforce hostname check
  CURLcode res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    LOG(ERROR) << "curl_easy_perform() failed: " << curl_easy_strerror(res);
    curl_easy_cleanup(curl);
    return std::nullopt;
  }
  long http_code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
  LOG(INFO) << "HTTP " << http_code << " from " << url;
  curl_easy_cleanup(curl);
  return response;
}

using FetchCallback = base::OnceCallback<void(std::optional<std::string>)>;

void FetchUrlAsync(const char* url, FetchCallback callback) {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(&FetchUrlBlocking, url), std::move(callback));
}
}  // namespace

int main(int argc, char* argv[]) {
  // The exit manager is in charge of calling the dtors of singletons.
  base::AtExitManager exit_manager;

  // Initialize the CommandLine singleton from the environment.
  base::CommandLine::Init(argc, argv);

  // Initialize logging settings based on command line arguments.
  logging::LoggingSettings settings;
  settings.logging_dest =
      logging::LOG_TO_SYSTEM_DEBUG_LOG | logging::LOG_TO_STDERR;
  logging::InitLogging(settings);

  // Start the IO thread.
  base::SingleThreadTaskExecutor task_executor(base::MessagePumpType::IO);
  base::ThreadPoolInstance::CreateAndStartWithDefaultParams("agent-poc");

  // Initialize Mojo IPC.
  mojo::core::Init();

  base::RunLoop loop;

  // Kick off the async fetch.  The lambda is invoked back on this thread
  // once the pool worker finishes, just like the SimpleURLLoader callback.
  FetchUrlAsync(kApiUrl,
                base::BindOnce(
                    [](base::RunLoop* loop, std::optional<std::string> body) {
                      if (body) {
                        LOG(INFO) << "Response body (" << body->size()
                                  << " bytes): " << body->substr(0, 1024);
                      } else {
                        LOG(ERROR) << "Request failed";
                      }
                      loop->Quit();
                    },
                    &loop));

  loop.Run();

  curl_global_cleanup();
  return 0;
}
