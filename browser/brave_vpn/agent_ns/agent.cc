#include <algorithm>

#include "base/at_exit.h"
#include "base/base64.h"
#include "base/check.h"
#include "base/command_line.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/logging/logging_settings.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/task/single_thread_task_executor.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "mojo/core/embedder/embedder.h"
#include "mojo/public/cpp/base/big_buffer.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "net/base/hash_value.h"
#include "net/base/isolation_info.h"
#include "net/base/net_errors.h"
#include "net/dns/public/dns_over_https_config.h"
#include "net/dns/public/dns_over_https_server_config.h"
#include "net/dns/public/secure_dns_mode.h"
#include "net/http/transport_security_state.h"
#include "net/url_request/url_request_context.h"
#include "services/cert_verifier/cert_verifier_service.h"
#include "services/cert_verifier/cert_verifier_service_factory.h"
#include "services/cert_verifier/public/mojom/cert_verifier_service_factory.mojom.h"
#include "services/network/network_context.h"
#include "services/network/network_service.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/cert_verifier_service.mojom.h"
#include "services/network/public/mojom/host_resolver.mojom.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "services/network/public/mojom/network_service.mojom.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace {
constexpr char kApiHost[] = "https://connect-api.guardianapp.com";
constexpr char kApiUrl[] =
    "https://connect-api.guardianapp.com/api/v1.3/servers/all-server-regions/"
    "city-by-country";
// Pinned Let's Encrypt E7 intermediate CA.
constexpr char kPinnedSPKIHash[] =
    "y7xVm0TVJNahMr2sZydE2jQH8SquXV9yLF9seROHHHU=";
constexpr size_t kMaxResponseBytes = 1024 * 1024;  // 1 MB cap

// Returns the raw CRLSet bytes from disk, or nullopt on failure.
std::optional<std::string> LoadLatestCRLSetBytes() {
  base::FilePath app_data_dir;

#if BUILDFLAG(IS_WIN)
  // %LOCALAPPDATA% e.g. C:\Users\<user>\AppData\Local
  if (!base::PathService::Get(base::DIR_LOCAL_APP_DATA, &app_data_dir)) {
#elif BUILDFLAG(IS_MAC)
  // ~/Library/Application Support
  if (!base::PathService::Get(base::DIR_APP_DATA, &app_data_dir)) {
#else  // Linux / other POSIX
  // HOME dir; we'll prepend .config below to follow XDG convention
  if (!base::PathService::Get(base::DIR_HOME, &app_data_dir)) {
#endif
    LOG(WARNING) << "Could not get app data dir";
    return std::nullopt;
  }

#if BUILDFLAG(IS_WIN)
  base::FilePath crl_base_dir = app_data_dir.AppendASCII(
      "BraveSoftware\\Brave-Browser\\User Data\\CertificateRevocation");
#elif BUILDFLAG(IS_MAC)
  base::FilePath crl_base_dir = app_data_dir.AppendASCII(
      "BraveSoftware/Brave-Browser/CertificateRevocation");
#else  // Linux
  base::FilePath crl_base_dir = app_data_dir.AppendASCII(
      ".config/BraveSoftware/Brave-Browser/CertificateRevocation");
#endif

  base::FilePath best_path;
  int best_sequence = -1;

  base::FileEnumerator enumerator(crl_base_dir, /*recursive=*/false,
                                  base::FileEnumerator::DIRECTORIES);
  for (base::FilePath subdir = enumerator.Next(); !subdir.empty();
       subdir = enumerator.Next()) {
    int sequence;
    if (base::StringToInt(subdir.BaseName().value(), &sequence) &&
        sequence > best_sequence) {
      base::FilePath candidate = subdir.AppendASCII("crl-set");
      if (base::PathExists(candidate)) {
        best_sequence = sequence;
        best_path = candidate;
      }
    }
  }

  if (best_path.empty()) {
    LOG(WARNING) << "No CRLSet found on disk";
    return std::nullopt;
  }

  std::string bytes;
  if (!base::ReadFileToString(best_path, &bytes)) {
    LOG(WARNING) << "Could not read CRLSet from " << best_path;
    return std::nullopt;
  }

  LOG(INFO) << "Loaded CRLSet (sequence dir " << best_sequence << ") from "
            << best_path;
  return bytes;
}

// Helper to decode a base64 SPKI hash into a net::HashValue.
net::HashValue SPKIHashFromBase64(const char* base64) {
  std::string bytes;
  CHECK(base::Base64Decode(base64, &bytes) && bytes.size() == 32u);
  return net::HashValue(base::as_byte_span(bytes));
}

// Stub client — required to avoid a DCHECK when the CRLSet update triggers
// OnCertVerifierChanged(). We don't need to act on the notification.
class StubCVServiceClient
    : public cert_verifier::mojom::CertVerifierServiceClient {
 public:
  StubCVServiceClient() : receiver_(this) {}
  void OnCertVerifierChanged() override {}
  mojo::PendingRemote<cert_verifier::mojom::CertVerifierServiceClient>
  BindAndPassRemote() {
    return receiver_.BindNewPipeAndPassRemote();
  }

 private:
  mojo::Receiver<cert_verifier::mojom::CertVerifierServiceClient> receiver_;
};
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

  // Create network service directly.
  mojo::Remote<network::mojom::NetworkService> network_service_remote;
  auto network_service = network::NetworkService::Create(
      network_service_remote.BindNewPipeAndPassReceiver());

  // Build DoH server config — using Cloudflare and Google as examples.
  // Replace with your own DoH server if you have one.
  auto dns_server = net::DnsOverHttpsServerConfig::FromString(
      "https://chrome.cloudflare-dns.com/dns-query");
  net::DnsOverHttpsConfig doh_config({*dns_server});

  network_service_remote->ConfigureStubHostResolver(
      /*insecure_dns_client_enabled=*/false,
      /*happy_eyeballs_v3_enabled=*/true, net::SecureDnsMode::kSecure,
      doh_config,
      /*additional_dns_types_enabled=*/false,
      /*fallback_doh_nameservers=*/{});

  // Create cert verifier service factory.
  mojo::Remote<cert_verifier::mojom::CertVerifierServiceFactory>
      cv_factory_remote;
  auto cv_factory =
      std::make_unique<cert_verifier::CertVerifierServiceFactoryImpl>(
          cv_factory_remote.BindNewPipeAndPassReceiver());
  CHECK(cv_factory);

  // Push the CRLSet to the factory — it applies to all CertVerifierServices
  // created by it, including ones created after this call.
  if (auto crl_bytes = LoadLatestCRLSetBytes()) {
    std::vector<uint8_t> buf(crl_bytes->begin(), crl_bytes->end());
    cv_factory->UpdateCRLSet(mojo_base::BigBuffer(std::move(buf)),
                             base::DoNothing());
  }

  // Create cert verifier service.
  StubCVServiceClient cv_client;
  mojo::PendingRemote<cert_verifier::mojom::CertVerifierService>
      cv_service_remote;
  // The second param is a CertVerifierServiceUpdater receiver — this is the
  // pipe through which the browser process would push future CRLSet/root store
  // updates to the verifier. Passing NullReceiver means the verifier is frozen
  // at startup and won't receive updates during the session.
  cv_factory->GetNewCertVerifier(
      cv_service_remote.InitWithNewPipeAndPassReceiver(), mojo::NullReceiver(),
      cv_client.BindAndPassRemote(),
      cert_verifier::mojom::CertVerifierCreationParams::New());

  // Create cert verifier params.
  auto cert_verifier_params =
      network::mojom::CertVerifierServiceRemoteParams::New();
  cert_verifier_params->cert_verifier_service = std::move(cv_service_remote);

  // Create network context with cert verifier params.
  auto ctx_params = network::mojom::NetworkContextParams::New();
  ctx_params->user_agent = "AgentPOC/1.0";
  ctx_params->cert_verifier_params = std::move(cert_verifier_params);
  mojo::Remote<network::mojom::NetworkContext> network_context_remote;
  auto network_context = std::make_unique<network::NetworkContext>(
      network_service.get(),
      network_context_remote.BindNewPipeAndPassReceiver(),
      std::move(ctx_params));
  CHECK(network_context);

  // Add pinning for the API host.
  net::TransportSecurityState* security_state =
      network_context->url_request_context()->transport_security_state();
  CHECK(security_state);
  net::HashValueVector pins = {
      SPKIHashFromBase64(kPinnedSPKIHash),  // E7 intermediate
  };
  security_state->AddHPKP("connect-api.guardianapp.com", base::Time::Max(),
                          /*include_subdomains=*/false, pins);

  // Print the PKP state to verify that the pin was added correctly.
  net::TransportSecurityState::PKPState pkp_state;
  if (security_state->GetDynamicPKPState("connect-api.guardianapp.com",
                                         &pkp_state)) {
    LOG(INFO) << "PKP active, " << pkp_state.spki_hashes.size() << " pins";
    for (const auto& hash : pkp_state.spki_hashes) {
      LOG(INFO) << "  " << base::Base64Encode(hash);
    }
  }

  // Create URL loader factory.
  const url::Origin api_origin = url::Origin::Create(GURL(kApiHost));
  auto factory_params = network::mojom::URLLoaderFactoryParams::New();
  factory_params->process_id = network::OriginatingProcess::browser();
  factory_params->is_trusted = true;
  factory_params->request_initiator_origin_lock = api_origin;

  mojo::Remote<network::mojom::URLLoaderFactory> url_loader_factory;
  network_context->CreateURLLoaderFactory(
      url_loader_factory.BindNewPipeAndPassReceiver(),
      std::move(factory_params));

  base::RunLoop loop;

  // HTTP request via SimpleURLLoader.
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = GURL(kApiUrl);
  resource_request->method = "GET";
  resource_request->trusted_params = network::ResourceRequest::TrustedParams();
  resource_request->trusted_params->isolation_info =
      net::IsolationInfo::CreateForInternalRequest(api_origin);
  resource_request->site_for_cookies =
      net::SiteForCookies::FromOrigin(api_origin);

  auto loader = network::SimpleURLLoader::Create(std::move(resource_request),
                                                 MISSING_TRAFFIC_ANNOTATION);
  loader->SetTimeoutDuration(base::Seconds(30));
  loader->DownloadToString(
      url_loader_factory.get(),
      base::BindOnce(
          [](base::RunLoop* loop, network::SimpleURLLoader* loader,
             std::optional<std::string> body) {
            if (loader->ResponseInfo() && loader->ResponseInfo()->headers) {
              LOG(ERROR) << "HTTP status: "
                         << loader->ResponseInfo()->headers->response_code();
            }
            if (!body) {
              LOG(ERROR) << "Request failed: "
                         << net::ErrorToString(loader->NetError()) << " ("
                         << loader->NetError() << ")";
            } else {
              LOG(INFO) << "Response body (" << body->size()
                        << " bytes): " << body->substr(0, 1024);
            }
            loop->Quit();
          },
          &loop, loader.get()),
      kMaxResponseBytes);

  loop.Run();
  return 0;
}
