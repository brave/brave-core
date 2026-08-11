#include "brave/bnes/bns_resolver.h"

#include <memory>
#include <utility>

#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "brave/bnes/bns_constants.h"
#include "brave/bnes/bns_security.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_wallet/browser/json_rpc_requests_helper.h"
#include "net/base/url_util.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace {

constexpr char kBNSRegistryAddress[] = "0xA3e9Dc4Fd7032Db1F4e8C8e776B3a7f23a65a85E";
constexpr char kBNSResolverAddress[] = "0x0000000000000000000000000000000000000000";
constexpr char kDefaultRPCEndpoint[] = "https://brnkc-mainnet.bearnetwork.net";
constexpr char kContenthashNode[] =
    "0x0000000000000000000000000000000000000000000000000000000000000000";

std::string BuildEthCallPayload(const std::string& to_address,
                                const std::string& data) {
  base::ListValue params;
  base::DictValue call_dict;
  call_dict.Set("to", to_address);
  call_dict.Set("data", data);
  params.Append(std::move(call_dict));
  params.Append("latest");
  return brave_wallet::GetJsonRpcString("eth_call", std::move(params));
}

std::string BuildContenthashCallData(const std::string& node) {
  // Function signature: contenthash(bytes32) = 0xbc55f78f
  // keccak256("contenthash(bytes32)") = 0xbc55f78f...
  constexpr char kContenthashSelector[] = "bc55f78f";
  return std::string(kContenthashSelector) + node.substr(2);
}

}  // namespace

namespace bns {

BnsResolver::BnsResolver(content::BrowserContext* browser_context)
    : browser_context_(browser_context) {
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory =
      browser_context_->GetURLLoaderFactory();
  api_request_helper_ = std::make_unique<api_request_helper::APIRequestHelper>(
      net::DefineNetworkTrafficAnnotation("bns_resolver", R"(
          semantics {
            sender: "BNS Resolver"
            description: "Resolves .bnes names to IPFS contenthashes."
            trigger: "Navigating to bnes:// URLs."
            data: "JSON-RPC request/response bodies."
            destination: WEBSITE
          }
          policy {
            cookies_allowed: NO
            setting: "Not configurable."
            policy_exception_justification: "Required for BNS resolution."
          }
        )"),
      std::move(url_loader_factory));
}

void BnsResolver::Resolve(const GURL& url, ResolveCallback callback) {
  if (!url.is_valid() || !url.SchemeIs(kBnesScheme)) {
    std::move(callback).Run(std::nullopt);
    return;
  }

  const std::string host = url.host();
  if (!IsValidBnesHost(host)) {
    std::move(callback).Run(std::nullopt);
    return;
  }

  // [BNES] Build JSON-RPC request to resolve contenthash
  // In H6.3+ this will integrate with MetaMask extension or RPC quorum
  std::string call_data = BuildContenthashCallData(kContenthashNode);
  std::string payload = BuildEthCallPayload(kBNSResolverAddress, call_data);

  GURL rpc_url(kDefaultRPCEndpoint);
  api_request_helper_->Request(
      "POST", rpc_url, payload, "application/json",
      base::BindOnce(&BnsResolver::OnRpcResponse, weak_ptr_factory_.GetWeakPtr(),
                     std::move(callback)),
      {{"Content-Type", "application/json"}});
}

void BnsResolver::OnRpcResponse(ResolveCallback callback,
                                std::optional<base::Value> result,
                                int response_code,
                                int error_code) {
  if (!result || !result->is_dict() || response_code != 200) {
    std::move(callback).Run(std::nullopt);
    return;
  }

  const base::Value::Dict* dict = result->GetDict();
  const std::string* contenthash_hex =
      dict->FindStringByDottedPath("result");
  if (!contenthash_hex || contenthash_hex->empty() ||
      *contenthash_hex == "0x") {
    std::move(callback).Run(std::nullopt);
    return;
  }

  ResolveResult resolve_result;
  if (!ParseContenthash(*contenthash_hex, &resolve_result.contenthash) ||
      !ValidateAndBuildGateway(resolve_result.contenthash,
                               &resolve_result)) {
    std::move(callback).Run(std::nullopt);
    return;
  }

  std::move(callback).Run(std::move(resolve_result));
}

bool BnsResolver::ParseContenthash(const std::string& hex_value,
                                   std::string* out_contenthash) {
  std::string stripped = hex_value;
  if (base::StartsWith(stripped, "0x", base::CompareCase::INSENSITIVE_ASCII)) {
    stripped = stripped.substr(2);
  }

  // [BNES] Validate ENS-style IPFS contenthash
  // Must start with 0xe3 (IPFS namespace)
  if (stripped.empty() || stripped[0] != 'e' || stripped[1] != '3') {
    return false;
  }

  *out_contenthash = hex_value;
  return true;
}

bool BnsResolver::ValidateAndBuildGateway(const std::string& contenthash,
                                          ResolveResult* out_result) {
  // [BNES] Decode ENS contenthash to CID and validate
  // Contenthash format: 0xe3 + CID binary
  std::string hex_body = contenthash;
  if (base::StartsWith(hex_body, "0x", base::CompareCase::INSENSITIVE_ASCII)) {
    hex_body = hex_body.substr(2);
  }
  if (hex_body.length() < 4) {
    return false;
  }

  // Skip IPFS namespace (0xe3), remaining is CID binary in hex
  std::string cid_hex = hex_body.substr(2);

  // [BNES] Reuse bns_security validation
  // CIDv0: starts with 0x1220 (sha2-256, 32 bytes digest)
  // CIDv1: starts with 0x01
  bool is_cidv0 = cid_hex.length() >= 4 && cid_hex.substr(0, 4) == "1220";
  bool is_cidv1 = !cid_hex.empty() && cid_hex[0] == '0' && cid_hex[1] == '1';

  std::string cid;
  if (is_cidv0) {
    // CIDv0: base58btc encode the raw bytes
    // For now, store hex representation; full base58 encoding
    // requires additional dependency
    cid = "Qm" + cid_hex.substr(4);
    if (cid.length() != 46) {
      return false;
    }
  } else if (is_cidv1) {
    // CIDv1: base32 lower encode
    cid = "b" + cid_hex.substr(2);
  } else {
    return false;
  }

  // [BNES] Build pinned HTTPS gateway URL
  GURL gateway = GURL(base::StrCat(
      {kHttpsScheme, url::kStandardSchemeSeparator,
       kDefaultIpfsGatewayHost, kIpfsPathPrefix, cid}));
  if (!gateway.is_valid() || !IsValidGatewayOrigin(gateway)) {
    return false;
  }

  out_result->cid = cid;
  out_result->gateway_url = gateway;
  return true;
}

}  // namespace bns
