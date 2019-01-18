/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "confirmations_impl.h"
#include "bat/confirmations/confirmations_client.h"
#include "bat/confirmations/issuer_info.h"
#include "logging.h"
#include "static_values.h"
#include "base/rand_util.h"
#include "chrome/browser/browser_process.h"

#include <vector>
#include <iostream>
#include <memory>
#include <mutex>
#include <condition_variable>

#include "base/base64.h"
#include "base/guid.h"
#include "base/time/time.h"
#include "net/base/escape.h"

#include "tweetnacl.h"
#include <openssl/base64.h>
#include <openssl/digest.h>
#include <openssl/hkdf.h>
#include <openssl/sha.h>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"

using namespace std::placeholders;

namespace confirmations {

ConfirmationsImpl::ConfirmationsImpl(
    ConfirmationsClient* confirmations_client) :
    is_initialized_(false),
    step_2_refill_confirmations_timer_id_(0),
    step_4_retrieve_payment_ious_timer_id_(0),
    step_5_cash_in_payment_ious_timer_id_(0),
    confirmations_client_(confirmations_client) {
  LoadState();
}

ConfirmationsImpl::~ConfirmationsImpl() {
  StopRefillingConfirmations();
  StopRetrievingPaymentIOUS();
  StopCashingInPaymentIOUS();
}

void ConfirmationsImpl::URLFetchSync(
    const std::string& url,
    const std::vector<std::string>& headers,
    const std::string& content,
    const std::string& content_type,
    net::URLFetcher::RequestType request_type) {
  net::URLFetcher* fetcher = net::URLFetcher::Create(
      GURL(url), request_type, this).release();
  fetcher->SetRequestContext(g_browser_process->system_request_context());

  for (size_t i = 0; i < headers.size(); i++) {
    fetcher->AddExtraRequestHeader(headers[i]);
  }

  if (!content.empty()) {
    fetcher->SetUploadData(content_type, content);
  }

  fetcher->Start();

  semaphore_.wait();
}

void ConfirmationsImpl::OnURLFetchComplete(
    const net::URLFetcher* source) {
  int response_code = source->GetResponseCode();
  std::string body;
  std::map<std::string, std::string> headers;
  scoped_refptr<net::HttpResponseHeaders> headersList =
      source->GetResponseHeaders();

  if (headersList) {
    size_t iter = 0;
    std::string key;
    std::string value;
    while (headersList->EnumerateHeaderLines(&iter, &key, &value)) {
      key = base::ToLowerASCII(key);
      headers[key] = value;
    }
  }

  if (response_code != net::URLFetcher::ResponseCode::RESPONSE_CODE_INVALID &&
      source->GetStatus().is_success()) {
    source->GetResponseAsString(&body);
  }

  delete source;

  response_ = body;
  response_code_ = response_code;
  semaphore_.signal();
}

void ConfirmationsImpl::VectorConcat(
    std::vector<std::string>* dest,
    std::vector<std::string>* source) {
  dest->insert(dest->end(), source->begin(), source->end());
}

std::unique_ptr<base::ListValue> ConfirmationsImpl::Munge(
    std::vector<std::string> v) {
  base::ListValue * list = new base::ListValue();

  for (auto x : v) {
    list->AppendString(x);
  }

  return std::unique_ptr<base::ListValue>(list);
}

std::vector<std::string> ConfirmationsImpl::Unmunge(base::Value* value) {
  std::vector<std::string> v;

  base::ListValue list(value->GetList());

  for (size_t i = 0; i < list.GetSize(); i++) {
    base::Value *x;
    list.Get(i, &x);
    v.push_back(x->GetString());
  }

  return v;
}

bool ConfirmationsImpl::IsReadyToShowAds() {
  // Whatever thread/service calls this in brave-core-client must also be the
  // one that triggers ad showing, or we'll have a race condition
  bool ready = (signed_blinded_confirmation_tokens.size() > 0);

  return ready;
}

std::string ConfirmationsImpl::GetServerUrl() {
  if (_is_production) {
    return BAT_ADS_PRODUCTION_SERVER;
  } else {
    return BAT_ADS_STAGING_SERVER;
  }
}

int ConfirmationsImpl::GetServerPort() {
  return BAT_ADS_SERVER_PORT;
}

void ConfirmationsImpl::Step1StoreTheServersConfirmationsPublicKeyAndGenerator(
    std::string confirmations_GH_pair,
    std::vector<std::string> bat_names,
    std::vector<std::string> bat_keys) {
  // This (G,H) *pair* is exposed as a *single* string via the rust lib G is the
  // generator the server used in H, see next line H, aka Y, is xG, the server's
  // public key These are both necessary for the DLEQ proof, but not useful
  // elsewhere These come back with the catalog from the server Later we'll get
  // an *array of pairs* for the payments side

  this->server_confirmation_key_ = confirmations_GH_pair;
  this->server_bat_payment_names = bat_names;
  this->server_bat_payment_keys = bat_keys;

  this->SaveState();

  BLOG(INFO) << "step1.1 : key: " << this->server_confirmation_key_;
}

std::string ConfirmationsImpl::toJSONString() {
  base::DictionaryValue dict;

  dict.SetKey("issuers_version",
      base::Value(issuers_version_));
  dict.SetKey("server_confirmation_key",
      base::Value(server_confirmation_key_));
  dict.SetWithoutPathExpansion("server_bat_payment_names",
      Munge(server_bat_payment_names));
  dict.SetWithoutPathExpansion("server_bat_payment_keys",
      Munge(server_bat_payment_keys));
  dict.SetWithoutPathExpansion("original_confirmation_tokens",
      Munge(original_confirmation_tokens));
  dict.SetWithoutPathExpansion("blinded_confirmation_tokens",
      Munge(blinded_confirmation_tokens));
  dict.SetWithoutPathExpansion("signed_blinded_confirmation_tokens",
      Munge(signed_blinded_confirmation_tokens));
  dict.SetWithoutPathExpansion("payment_token_json_bundles",
      Munge(payment_token_json_bundles));
  dict.SetWithoutPathExpansion("signed_blinded_payment_token_json_bundles",
      Munge(signed_blinded_payment_token_json_bundles));
  dict.SetWithoutPathExpansion("fully_submitted_payment_bundles",
      Munge(fully_submitted_payment_bundles));

  std::string json;
  base::JSONWriter::Write(dict, &json);

  return json;
}

void ConfirmationsImpl::Step2RefillConfirmationsIfNecessary(
    std::string real_wallet_address,
    std::string real_wallet_address_secret_key,
    std::string local_server_confirmation_key) {
  if (this->blinded_confirmation_tokens.size() > low_token_threshold) {
    BLOG(INFO) << "Not necessary to refill confirmations";

    return;
  }

  std::vector<std::string> local_original_confirmation_tokens = {};
  std::vector<std::string> local_blinded_confirmation_tokens = {};

  size_t needed = refill_amount - blinded_confirmation_tokens.size();

  for (size_t i = 0; i < needed; i++) {
    // client prepares a random token and blinding scalar pair
    Token token = Token::random();
    std::string token_base64 = token.encode_base64();

    // client blinds the token
    BlindedToken blinded_token = token.blind();
    std::string blinded_token_base64 = blinded_token.encode_base64();

    // client stores the original token and the blinded token
    // will send blinded token to server
    local_original_confirmation_tokens.push_back(token_base64);
    local_blinded_confirmation_tokens.push_back(blinded_token_base64);
  }

  BLOG(INFO) << "step2.1 : batch generate, count: "
      << local_original_confirmation_tokens.size();

  {
    std::string digest = "digest";
    std::string primary = "primary";

    ///////////////////////////////////////////////////////////////////////////

    std::string build = "";

    build.append("{\"blindedTokens\":");
    build.append("[");
    std::vector<std::string> a = local_blinded_confirmation_tokens;

    for (size_t i = 0; i < a.size(); i++) {
      if (i > 0) {
        build.append(",");
      }
      build.append("\"");
      build.append(a[i]);
      build.append("\"");
    }

    build.append("]");
    build.append("}");

    std::string real_body = build;

    std::vector<uint8_t> real_sha_raw = this->GetSHA256(real_body);
    std::string real_body_sha_256_b64 = this->GetBase64(real_sha_raw);

    std::vector<uint8_t> real_skey = this->RawDataBytesVectorFromASCIIHexString(
        real_wallet_address_secret_key);

    std::string real_digest_field = std::string("SHA-256=").append(
        real_body_sha_256_b64);

    std::string real_signature_field = this->Sign(
        &digest, &real_digest_field, 1, primary, real_skey);

    ///////////////////////////////////////////////////////////////////////////
    std::string endpoint = std::string("/v1/confirmation/token/").append(
        real_wallet_address);
    std::string url = GetServerUrl().append(endpoint);
    std::vector<std::string> headers = {};
    headers.push_back(std::string("digest: ").append(real_digest_field));
    headers.push_back(std::string("signature: ").append(real_signature_field));
    headers.push_back(std::string("accept: ").append("application/json"));
    std::string content_type = "application/json";
    URLFetchSync(url, headers, real_body, content_type,
        net::URLFetcher::RequestType::POST);

    // This should be the `nonce` in the return. we need to
    // make sure we get the nonce in the separate request observation. seems
    // like we should move all of this (the tokens in-progress) data to a map
    // keyed on the nonce, and then step the storage through (pump) in a
    // state-wise (dfa) as well, so the storage types are coded (named) on a
    // dfa-state-respecting basis

    std::unique_ptr<base::Value> value(base::JSONReader::Read(response_));
    base::DictionaryValue* dict;
    if (!value->GetAsDictionary(&dict)) {
      BLOG(ERROR) << "2.2 post resp: no dict" << "\n";
      return;
    }

    base::Value *v;
    if (!(v = dict->FindKey("nonce"))) {
      BLOG(ERROR) << "2.2 no nonce\n";
      return;
    }

    std::string nonce = v->GetString();

    // Instead of pursuing true asynchronicity at this point, what we can do is
    // sleep for a minute or two and blow away any work to this point on failure
    // this solves the problem for now since the tokens have no value at this
    // point

    // STEP 2.3 This is done blocking and assumes success but we need to
    // separate it more and account for the possibility of failures

    {
      BLOG(INFO) << "step2.3 : GET /v1/confirmation/token/{payment_id}?nonce=: "
          << nonce;

      std::string endpoint = std::string("/v1/confirmation/token/").append(
          real_wallet_address).append("?nonce=").append(nonce);
      std::string url = GetServerUrl().append(endpoint);
      URLFetchSync(url, {}, "", "", net::URLFetcher::RequestType::GET);

      /////////////////////////////////////////////////////////////////////////

      // response_: {"batchProof":"r2qx2h5ENHASgBxEhN2TjUjtC2L2McDN6g/lZ+nTaQ6q+
      // 6TZH0InhxRHIp0vdUlSbMMCHaPdLYsj/IJbseAtCw==","signedTokens":["VI27MCax4
      // V9Gk60uC1dwCHHExHN2WbPwwlJk87fYAyo=","mhFmcWHLk5X8v+a/X0aea24OfGWsfAwWb
      // P7RAeXXLV4="]}

      std::unique_ptr<base::Value> value(base::JSONReader::Read(response_));
      base::DictionaryValue* dict;
      if (!value->GetAsDictionary(&dict)) {
        BLOG(ERROR) << "2.3 get resp: no dict" << "\n";
        return;
      }

      base::Value *v;

      if (!(v = dict->FindKey("batchProof"))) {
        BLOG(ERROR) << "2.3 no batchProof\n";
        return;
      }

      std::string real_batch_proof = v->GetString();

      if (!(v = dict->FindKey("signedTokens"))) {
        BLOG(ERROR) << "2.3 no signedTokens\n";
        return;
      }

      base::ListValue list(v->GetList());

      std::vector<std::string> server_signed_blinded_confirmations = {};

      for (size_t i = 0; i < list.GetSize(); i++) {
        base::Value *x;
        list.Get(i, &x);

        auto sbc = x->GetString();

        server_signed_blinded_confirmations.push_back(sbc);
      }

      bool real_verified = this->VerifyBatchDLEQProof(
          real_batch_proof,
          local_blinded_confirmation_tokens,
          server_signed_blinded_confirmations,
          local_server_confirmation_key);
      if (!real_verified) {
        BLOG(ERROR) << "ERROR: Server confirmations proof invalid";
        return;
      }

      // finally, if everything succeeded we'll modify object state and
      // persist
      BLOG(INFO) <<
          "step2.4 : store the signed blinded confirmations tokens & pre data";

      VectorConcat(&this->original_confirmation_tokens,
          &local_original_confirmation_tokens);
      VectorConcat(&this->blinded_confirmation_tokens,
          &local_blinded_confirmation_tokens);
      VectorConcat(&this->signed_blinded_confirmation_tokens,
          &server_signed_blinded_confirmations);
      this->SaveState();
    }  // 2.3
  }  // 2.1
}

void ConfirmationsImpl::Step3RedeemConfirmation(
    std::string real_creative_instance_id) {
  if (this->signed_blinded_confirmation_tokens.size() <= 0) {
    BLOG(INFO) << "ERROR: step 3.1a, no signed blinded confirmation tokens";
    return;
  }

  BLOG(INFO) << "step3.1a: unblinding signed blinded confirmations";

  std::string orig_token_b64 = this->original_confirmation_tokens.front();
  std::string sb_token_b64 = this->signed_blinded_confirmation_tokens.front();

  // rehydrate
  Token restored_token = Token::decode_base64(orig_token_b64);
  SignedToken signed_token = SignedToken::decode_base64(sb_token_b64);

  // use blinding scalar to unblind
  UnblindedToken client_unblinded_token = restored_token.unblind(signed_token);

  // dehydrate
  std::string base64_unblinded_token = client_unblinded_token.encode_base64();

  std::string local_unblinded_signed_confirmation_token =
      base64_unblinded_token;

  // we're doing this here instead of doing it on success and tracking
  // success/failure since it's cheaper development wise. but optimization wise,
  // it "wastes" a (free) confirmation token on failure
  this->PopFrontConfirmation();

  // persist
  this->SaveState();

  BLOG(INFO) << "step3.1b: generate payment, count: "
      << original_confirmation_tokens.size();

  // client prepares a random token and blinding scalar pair
  Token token = Token::random();
  std::string token_base64 = token.encode_base64();

  // client blinds the token
  BlindedToken blinded_token = token.blind();
  std::string blinded_token_base64 = blinded_token.encode_base64();

  std::string local_original_payment_token = token_base64;
  std::string local_blinded_payment_token = blinded_token_base64;

  // what's `t`? local_unblinded_signed_confirmation_token
  // what's `MAC_{sk}(R)`? item from blinded_payment_tokens

  // prePaymentToken changed to blindedPaymentToken
  std::string blindedPaymentToken = local_blinded_payment_token;
  std::string json;

  // build body of POST request
  base::DictionaryValue dict;
  dict.SetKey("creativeInstanceId", base::Value(real_creative_instance_id));
  dict.SetKey("payload", base::Value(base::Value::Type::DICTIONARY));
  dict.SetKey("blindedPaymentToken", base::Value(blindedPaymentToken));
  dict.SetKey("type", base::Value("landed"));
  base::JSONWriter::Write(dict, &json);

  UnblindedToken restored_unblinded_token = UnblindedToken::decode_base64(
      local_unblinded_signed_confirmation_token);
  VerificationKey client_vKey =
      restored_unblinded_token.derive_verification_key();
  std::string message = json;
  VerificationSignature client_sig = client_vKey.sign(message);

  std::string base64_token_preimage =
      restored_unblinded_token.preimage().encode_base64();
  std::string base64_signature = client_sig.encode_base64();

  base::DictionaryValue bundle;
  std::string credential_json;
  bundle.SetKey("payload", base::Value(json));
  bundle.SetKey("signature", base::Value(base64_signature));
  bundle.SetKey("t", base::Value(base64_token_preimage));
  base::JSONWriter::Write(bundle, &credential_json);

  std::vector<uint8_t> vec(credential_json.begin(), credential_json.end());
  std::string b64_encoded_a = this->GetBase64(vec);

  std::string b64_encoded;
  base::Base64Encode(credential_json, &b64_encoded);

  DCHECK(b64_encoded_a == b64_encoded);

  std::string uri_encoded = net::EscapeQueryParamValue(b64_encoded, true);

  // 3 pieces we need for our POST request, 1 for URL, 1 for body, and 1 for URL
  // that depends on body
  std::string confirmation_id = base::GenerateGUID();
  std::string real_body = json;
  std::string credential = uri_encoded;

  /////////////////////////////////////////////////////////////////////////////

  // step_3_1c POST /v1/confirmation/{confirmation_id}/{credential}, which is
  // (t, MAC_(sk)(R))
  BLOG(INFO) <<
      "step3.1c: POST /v1/confirmation/{confirmation_id}/{credential} "
      << confirmation_id;

  std::string endpoint = std::string("/v1/confirmation/").append(
      confirmation_id).append("/").append(credential);
  std::string url = GetServerUrl().append(endpoint);
  std::vector<std::string> headers = {};
  headers.push_back(std::string("accept: ").append("application/json"));
  std::string content_type = "application/json";
  URLFetchSync(url, headers, real_body, content_type,
      net::URLFetcher::RequestType::POST);

  /////////////////////////////////////////////////////////////////////////////

  if (response_code_ == 201) {  // 201 - created
    std::unique_ptr<base::Value> value(base::JSONReader::Read(response_));
    base::DictionaryValue* dict;
    if (!value->GetAsDictionary(&dict)) {
      BLOG(ERROR) << "no 3.1c resp dict" << "\n";
      return;
    }

    base::Value *v;
    if (!(v = dict->FindKey("id"))) {
      BLOG(ERROR) << "3.1c could not get id\n";
      return;
    }

    std::string id31 = v->GetString();
    DCHECK(confirmation_id == id31);

    // check return code, check json for `id` key
    // for bundle:
    //   ✓ confirmation_id
    //   ✓ local_original_payment_token
    //   ✓ local_blinded_payment_token - we do need: for DLEQ proof
    //   ✗ bundle_timestamp - nice to have in case we want to expire later

    std::string timestamp = std::to_string(
        base::Time::NowFromSystemTime().ToTimeT());

    BLOG(INFO) << "step3.2 : store confirmationId &such";

    base::DictionaryValue bundle;
    bundle.SetKey("confirmation_id",
        base::Value(confirmation_id));
    bundle.SetKey("original_payment_token",
        base::Value(local_original_payment_token));
    bundle.SetKey("blinded_payment_token",
        base::Value(local_blinded_payment_token));
    bundle.SetKey("bundle_timestamp",
        base::Value(timestamp));

    std::string bundle_json;
    base::JSONWriter::Write(bundle, &bundle_json);
    this->payment_token_json_bundles.push_back(bundle_json);
    this->SaveState();
  }
}

  bool ConfirmationsImpl::ProcessIOUBundle(std::string bundle_json) {
    bool unfinished = false;
    bool finished   = true;

    std::string confirmation_id;
    std::string original_payment_token;
    std::string blinded_payment_token;

    std::unique_ptr<base::Value> bundle_value(
        base::JSONReader::Read(bundle_json));
    base::DictionaryValue* map;
    if (!bundle_value->GetAsDictionary(&map)) {
      BLOG(ERROR) << "no 4 process iou bundle dict" << "\n";
      return finished;
    }

    base::Value *u;

    if (!(u = map->FindKey("confirmation_id"))) {
      BLOG(ERROR) << "4 process iou bundle, could not get confirmation_id";
      return finished;
    }
    confirmation_id = u->GetString();

    if (!(u = map->FindKey("original_payment_token"))) {
      BLOG(ERROR) <<
          "4 process iou bundle, could not get original_payment_token";
      return finished;
    }
    original_payment_token = u->GetString();

    if (!(u = map->FindKey("blinded_payment_token"))) {
      BLOG(ERROR) <<
          "4 process iou bundle, could not get blinded_payment_token";
      return finished;
    }
    blinded_payment_token = u->GetString();

    // 4.1 GET /v1/confirmation/{confirmation_id}/paymentToken
    BLOG(INFO) <<
        "step4.1 : GET /v1/confirmation/{confirmation_id}/paymentToken";

    std::string endpoint = std::string("/v1/confirmation/").append(
        confirmation_id).append("/paymentToken");
    std::string url = GetServerUrl().append(endpoint);
    URLFetchSync(url, {}, "", "", net::URLFetcher::RequestType::GET);

    if (!(response_code_ == 200 || response_code_ == 202)) {
      // something broke before server could decide paid:true/false
      BLOG(ERROR) << "ProcessIOUBundle response code: " << response_code_;

      return unfinished;
    }

    // 2018.12.10 apparently, server side has changed to always pay tokens, so
    // we won't recv 202 response?
    if (response_code_ == 202) {  // paid:false response
      // 1. collect estimateToken from JSON
      // 2. derive estimate

      std::unique_ptr<base::Value> value(base::JSONReader::Read(response_));
      base::DictionaryValue* dict;
      if (!value->GetAsDictionary(&dict)) {
        BLOG(ERROR) << "4.1 202 no dict" << "\n";
        return unfinished;
      }

      base::Value *v;
      if (!(v = dict->FindKey("estimateToken"))) {
        BLOG(ERROR) << "4.1 202 no estimateToken\n";
        return unfinished;
      }

      base::DictionaryValue* et;
      if (!v->GetAsDictionary(&et)) {
        BLOG(ERROR) << "4.1 202 no eT dict" << "\n";
        return unfinished;
      }

      if (!(v = et->FindKey("publicKey"))) {
        BLOG(ERROR) << "4.1 202 no publicKey\n";
        return unfinished;
      }

      std::string token = v->GetString();
      std::string name = this->BATNameFromBATPublicKey(token);
      if (name != "") {
        std::string estimated_payment_worth = name;
      } else {
        BLOG(ERROR) << "Step 4.1 202 verification empty name \n";
      }

      return unfinished;
    }

    if (response_code_ == 200) {  // paid:true response
      base::Value *v;
      std::unique_ptr<base::Value> value(base::JSONReader::Read(response_));
      base::DictionaryValue* dict;
      if (!value->GetAsDictionary(&dict)) {
        BLOG(ERROR) << "4.1 200 no dict" << "\n";
        return unfinished;
      }

      if (!(v = dict->FindKey("id"))) {
        BLOG(ERROR) << "4.1 200 no id\n";
        return unfinished;
      }
      std::string id = v->GetString();

      if (!(v = dict->FindKey("paymentToken"))) {
        BLOG(ERROR) << "4.1 200 no paymentToken\n";
        return unfinished;
      }

      base::DictionaryValue* pt;
      if (!v->GetAsDictionary(&pt)) {
        BLOG(ERROR) << "4.1 200 no pT dict" << "\n";
        return unfinished;
      }

      if (!(v = pt->FindKey("publicKey"))) {
        BLOG(ERROR) << "4.1 200 no publicKey\n";
        return unfinished;
      }
      std::string publicKey = v->GetString();

      if (!(v = pt->FindKey("batchProof"))) {
        BLOG(ERROR) << "4.1 200 no batchProof\n";
        return unfinished;
      }
      std::string batchProof = v->GetString();

      if (!(v = pt->FindKey("signedTokens"))) {
        BLOG(ERROR) << "4.1 200 could not get signedTokens\n";
        return unfinished;
      }

      base::ListValue signedTokensList(v->GetList());
      std::vector<std::string> signedBlindedTokens = {};

      if (signedTokensList.GetSize() != 1) {
        BLOG(ERROR) <<
            "4.1 200 currently unsupported size for signedTokens array\n";
        return unfinished;
      }

      for (size_t i = 0; i < signedTokensList.GetSize(); i++) {
        base::Value *x;
        signedTokensList.Get(i, &x);
        signedBlindedTokens.push_back(x->GetString());
      }

      std::vector<std::string> local_blinded_payment_tokens =
          {blinded_payment_token};

      bool real_verified = this->VerifyBatchDLEQProof(
          batchProof,
          local_blinded_payment_tokens,
          signedBlindedTokens,
          publicKey);

      if (!real_verified) {
        // 2018.11.29 kevin - ok to log these only (maybe forever) but don't
        // consider failing until after we're versioned on "issuers" private
        // keys
        BLOG(ERROR) << "ERROR: Real payment proof invalid";
      }

      std::string name = this->BATNameFromBATPublicKey(publicKey);
      std::string payment_worth = "";
      if (name != "") {
        payment_worth = name;
      } else {
        BLOG(ERROR) << "Step 4.1/4.2 200 verification empty name \n";
      }

      for (auto signedBlindedPaymentToken : signedBlindedTokens) {
        BLOG(INFO) << "step4.2 : store signed blinded payment token";
        map->SetKey("signed_blinded_payment_token",
             base::Value(signedBlindedPaymentToken));
        map->SetKey("server_payment_key", base::Value(publicKey));
        map->SetKey("payment_worth", base::Value(payment_worth));

        std::string json_with_signed;
        base::JSONWriter::Write(*map, &json_with_signed);

        this->signed_blinded_payment_token_json_bundles.push_back(
            json_with_signed);
        this->SaveState();
      }

      return finished;
    }

    return unfinished;
  }

  void ConfirmationsImpl::Step4RetrievePaymentIOUs() {
    // we cycle through this multiple times until the token is marked paid
    std::vector<std::string> remain = {};
    for (auto payment_bundle_json : this->payment_token_json_bundles) {
      bool finished = ProcessIOUBundle(payment_bundle_json);
      if (!finished) {
          remain.push_back(payment_bundle_json);
      }
    }
    this->payment_token_json_bundles = remain;
    this->SaveState();
  }

  void ConfirmationsImpl::Step5CashInPaymentIOUs(
      std::string real_wallet_address) {
    BLOG(INFO) << "step5.1 : unblind signed blinded payments";

    size_t n = this->signed_blinded_payment_token_json_bundles.size();

    if (n <= 0) {
      return;
    }

    std::vector<std::string> local_unblinded_signed_payment_tokens = {};
    std::vector<std::string> local_payment_keys = {};

    for (size_t i = 0; i < n; i++) {
      std::string json = this->signed_blinded_payment_token_json_bundles[i];

      std::unique_ptr<base::Value> value(base::JSONReader::Read(json));
      base::DictionaryValue* map;
      if (!value->GetAsDictionary(&map)) {
        BLOG(ERROR) << "5 cannot rehydrate: no map" << "\n";
        return;
      }

      base::Value *u;

      if (!(u = map->FindKey("server_payment_key"))) {
        BLOG(ERROR) << "5 process iou bundle, could not get server_payment_key";
        return;
      }
      std::string server_payment_key = u->GetString();

      if (!(u = map->FindKey("original_payment_token"))) {
        BLOG(ERROR) <<
            "5 process iou bundle, could not get original_payment_token";
        return;
      }
      std::string original_payment_token = u->GetString();

      if (!(u = map->FindKey("signed_blinded_payment_token"))) {
        BLOG(ERROR) <<
            "5 process iou bundle, could not get signed_blinded_payment_token";
        return;
      }
      std::string signed_blinded_payment_token = u->GetString();

      std::string orig_token_b64 = original_payment_token;
      std::string sb_token_b64 = signed_blinded_payment_token;

      // rehydrate
      Token restored_token = Token::decode_base64(orig_token_b64);
      SignedToken signed_token = SignedToken::decode_base64(sb_token_b64);
      // use blinding scalar to unblind
      UnblindedToken client_unblinded_token =
          restored_token.unblind(signed_token);
      // dehydrate
      std::string base64_unblinded_token =
          client_unblinded_token.encode_base64();
      // put on object
      local_unblinded_signed_payment_tokens.push_back(base64_unblinded_token);
      local_payment_keys.push_back(server_payment_key);
    }

    // PUT /v1/confirmation/token/{payment_id}
    std::string endpoint = std::string("/v1/confirmation/payment/").append(
        real_wallet_address);

    // {}->payload->{}->payment_id
    // real_wallet_address

    // {}->paymentCredentials->[]->{}->credential->{}->signature
    // signature of payload

    // {}->paymentCredentials->[]->{}->credential->{}->t
    // uspt

    // {}->paymentCredentials->[]->{}->publicKey
    // server_payment_key

    std::string primary = "primary";
    std::string pay_key = "paymentId";
    std::string pay_val = real_wallet_address;

    base::DictionaryValue payload;
    payload.SetKey(pay_key, base::Value(pay_val));

    std::string payload_json;
    base::JSONWriter::Write(payload, &payload_json);

    base::ListValue * list = new base::ListValue();

    for (size_t i = 0; i < local_unblinded_signed_payment_tokens.size(); i++) {
      auto uspt = local_unblinded_signed_payment_tokens[i];
      std::string server_payment_key = local_payment_keys[i];

      UnblindedToken restored_unblinded_token =
          UnblindedToken::decode_base64(uspt);
      VerificationKey client_vKey =
          restored_unblinded_token.derive_verification_key();
      std::string message = payload_json;
      VerificationSignature client_sig = client_vKey.sign(message);
      std::string base64_signature = client_sig.encode_base64();
      std::string base64_token_preimage =
          restored_unblinded_token.preimage().encode_base64();

      base::DictionaryValue cred;
      cred.SetKey("signature", base::Value(base64_signature));
      cred.SetKey("t", base::Value(base64_token_preimage));
      // cred.SetKey("t", base::Value(uspt));

      base::DictionaryValue * dict = new base::DictionaryValue();
      dict->SetKey("credential", std::move(cred));
      dict->SetKey("publicKey", base::Value(server_payment_key));

      list->Append(std::unique_ptr<base::DictionaryValue>(dict));
    }

    base::DictionaryValue sdict;
    sdict.SetWithoutPathExpansion("paymentCredentials",
        std::unique_ptr<base::ListValue>(list));
    // sdict.SetKey("payload", std::move(payload));
    sdict.SetKey("payload", base::Value(payload_json));

    std::string json;
    base::JSONWriter::Write(sdict, &json);
    std::string real_body = json;

    std::string url = GetServerUrl().append(endpoint);
    std::vector<std::string> headers = {};
    headers.push_back(std::string("accept: ").append("application/json"));
    std::string content_type = "application/json";
    URLFetchSync(url, headers, real_body, content_type,
        net::URLFetcher::RequestType::PUT);

    if (response_code_ != 200) {
      BLOG(ERROR) << "Step5CashInPaymentIOUs response code: " << response_code_;
      return;
    }

    if (response_code_ == 200) {
      BLOG(INFO) << "step5.2 : store txn ids and actual payment";

      VectorConcat(&this->fully_submitted_payment_bundles,
          &this->signed_blinded_payment_token_json_bundles);
      this->signed_blinded_payment_token_json_bundles.clear();
      this->SaveState();
    }
  }

  bool ConfirmationsImpl::VerifyBatchDLEQProof(
      std::string proof_string,
      std::vector<std::string> blinded_strings,
      std::vector<std::string> signed_strings,
      std::string public_key_string) {
    bool failure = 0;
    bool success = 1;

    BatchDLEQProof batch_proof = BatchDLEQProof::decode_base64(proof_string);

    std::vector<BlindedToken> blinded_tokens;
    for (auto x : blinded_strings) {
      blinded_tokens.push_back(BlindedToken::decode_base64(x));
    }

    std::vector<SignedToken> signed_tokens;
    for (auto x : signed_strings) {
      signed_tokens.push_back(SignedToken::decode_base64(x));
    }

    PublicKey public_key = PublicKey::decode_base64(public_key_string);

    if (!batch_proof.verify(blinded_tokens, signed_tokens, public_key)) {
      return failure;
    }

    return success;
  }

  void ConfirmationsImpl::PopFrontConfirmation() {
    auto &a = this->original_confirmation_tokens;
    auto &b = this->blinded_confirmation_tokens;
    auto &c = this->signed_blinded_confirmation_tokens;

    a.erase(a.begin());
    b.erase(b.begin());
    c.erase(c.begin());
  }

  void ConfirmationsImpl::PopFrontPayment() {
    auto &a = this->signed_blinded_payment_token_json_bundles;

    a.erase(a.begin());
  }

  std::string ConfirmationsImpl::BATNameFromBATPublicKey(std::string token) {
    std::vector<std::string> &k = this->server_bat_payment_keys;

    // find position of public key in the BAT array (later use same pos to find
    // the `name`)
    ptrdiff_t pos = distance(k.begin(), find(k.begin(), k.end(), token));

    bool found = pos < (ptrdiff_t)k.size();

    if (!found) {
      return "";
    }

    std::string name = this->server_bat_payment_names[pos];
    return name;
  }

  std::string ConfirmationsImpl::Sign(
      std::string* keys,
      std::string* values,
      const unsigned int& size,
      const std::string& keyId,
      const std::vector<uint8_t>& secretKey) {
    std::string headers;
    std::string message;
    for (unsigned int i = 0; i < size; i++) {
      if (0 != i) {
        headers += " ";
        message += "\n";
      }
      headers += keys[i];
      message += keys[i] + ": " + values[i];
    }
    std::vector<uint8_t> signedMsg(crypto_sign_BYTES + message.length());

    unsigned long long signedMsgSize = 0;
    crypto_sign(&signedMsg.front(), &signedMsgSize,
        (const unsigned char*)message.c_str(),
        (unsigned long long)message.length(), &secretKey.front());

    std::vector<uint8_t> signature(crypto_sign_BYTES);
    std::copy(signedMsg.begin(), signedMsg.begin() +
        crypto_sign_BYTES, signature.begin());

    return "keyId=\"" + keyId + "\",algorithm=\"" +
        CONFIRMATIONS_SIGNATURE_ALGORITHM +
        "\",headers=\"" + headers + "\",signature=\"" +
            GetBase64(signature) + "\"";
  }

  std::vector<uint8_t> ConfirmationsImpl::GetSHA256(const std::string& in) {
    std::vector<uint8_t> res(SHA256_DIGEST_LENGTH);
    SHA256((uint8_t*)in.c_str(), in.length(), &res.front());
    return res;
  }

  std::string ConfirmationsImpl::GetBase64(const std::vector<uint8_t>& in) {
    std::string res;
    size_t size = 0;
    if (!EVP_EncodedLength(&size, in.size())) {
      DCHECK(false);
      BLOG(ERROR) << "EVP_EncodedLength failure in GetBase64";

      return "";
    }
    std::vector<uint8_t> out(size);
    int numEncBytes = EVP_EncodeBlock(&out.front(), &in.front(), in.size());
    DCHECK_NE(numEncBytes, 0);
    res = (char*)&out.front();
    return res;
  }

  std::vector<uint8_t> ConfirmationsImpl::RawDataBytesVectorFromASCIIHexString(
      std::string ascii) {
    std::vector<uint8_t> bytes;
    size_t len = ascii.length();
    for (size_t i = 0; i < len; i += 2) {
        std::string b =  ascii.substr(i, 2);
        uint8_t x = std::strtol(b.c_str(), 0, 16);
        bytes.push_back(x);
    }
    return bytes;
  }

  bool ConfirmationsImpl::FromJSONString(std::string json_string) {
    bool fail = 0;
    bool succeed = 1;

    std::unique_ptr<base::Value> value(base::JSONReader::Read(json_string));

    if (!value) {
      return fail;
    }

    base::DictionaryValue* dict;
    if (!value->GetAsDictionary(&dict)) {
      return fail;
    }

    base::Value *v;

    if (!(v = dict->FindKey("issuers_version"))) return fail;
    this->issuers_version_ = v->GetString();

    if (!(v = dict->FindKey("server_confirmation_key"))) return fail;
    this->server_confirmation_key_ = v->GetString();

    if (!(v = dict->FindKey("server_bat_payment_names"))) return fail;
    this->server_bat_payment_names = Unmunge(v);

    if (!(v = dict->FindKey("server_bat_payment_keys"))) return fail;
    this->server_bat_payment_keys = Unmunge(v);

    if (!(v = dict->FindKey("original_confirmation_tokens"))) return fail;
    this->original_confirmation_tokens = Unmunge(v);

    if (!(v = dict->FindKey("blinded_confirmation_tokens"))) return fail;
    this->blinded_confirmation_tokens = Unmunge(v);

    if (!(v = dict->FindKey("signed_blinded_confirmation_tokens"))) return fail;
    this->signed_blinded_confirmation_tokens = Unmunge(v);

    if (!(v = dict->FindKey("signed_blinded_payment_token_json_bundles")))
        return fail;
    this->signed_blinded_payment_token_json_bundles = Unmunge(v);

    if (!(v = dict->FindKey("fully_submitted_payment_bundles"))) return fail;
    this->fully_submitted_payment_bundles = Unmunge(v);

    return succeed;
  }

  void ConfirmationsImpl::SaveState() {
    std::string json = toJSONString();
    auto callback = std::bind(&ConfirmationsImpl::OnStateSaved, this, _1);
    confirmations_client_->Save(_confirmations_name, json, callback);
  }

  void ConfirmationsImpl::OnStateSaved(const Result result) {
    if (result == FAILED) {
      BLOG(ERROR) << "Failed to save confirmations state";

      return;
    }

    BLOG(INFO) << "Successfully saved confirmations state";
  }

  void ConfirmationsImpl::LoadState() {
    auto callback = std::bind(&ConfirmationsImpl::OnStateLoaded, this, _1, _2);
    confirmations_client_->Load(_confirmations_name, callback);
  }

  void ConfirmationsImpl::OnStateLoaded(
      const Result result,
      const std::string& json) {
    if (result == FAILED) {
      BLOG(ERROR) << "Failed to load confirmations state";
      return;
    }

    if (!FromJSONString(json)) {
      BLOG(ERROR) << "Failed to parse confirmations state: " << json;
      return;
    }

    BLOG(INFO) << "Successfully loaded confirmations state";

    is_initialized_ = true;

    RefillConfirmations();
    RetrievePaymentIOUS();
    CashInPaymentIOUS();
  }

  void ConfirmationsImpl::ResetState() {
    BLOG(INFO) << "Resetting confirmations to default state";

    auto callback = std::bind(&ConfirmationsImpl::OnStateReset, this, _1);
    confirmations_client_->Reset(_confirmations_name, callback);
  }

  void ConfirmationsImpl::OnStateReset(const Result result) {
    if (result == FAILED) {
      BLOG(ERROR) << "Failed to reset confirmations state";

      return;
    }

    BLOG(INFO) << "Successfully reset confirmations state";
  }

  /////////////////////////////////////////////////////////////////////////////

  MockServer::MockServer() {
  }

  MockServer::~MockServer() {
  }

  void MockServer::Test() {
  }

  void MockServer::GenerateSignedBlindedTokensAndProof(
      std::vector<std::string> blinded_tokens) {
    std::vector<std::string> stamped;

    std::vector<BlindedToken> rehydrated_blinded_tokens;
    std::vector<SignedToken>  rehydrated_signed_tokens;

    for (auto x : blinded_tokens) {
      // rehydrate the token from the base64 string
      BlindedToken blinded_token = BlindedToken::decode_base64(x);

      // keep it for the proof later
      rehydrated_blinded_tokens.push_back(blinded_token);

      // server signs the blinded token
      SignedToken signed_token = this->signing_key.sign(blinded_token);

      // keep it for the proof later
      rehydrated_signed_tokens.push_back(signed_token);

      std::string base64_signed_token = signed_token.encode_base64();
      stamped.push_back(base64_signed_token);
    }

    BatchDLEQProof server_batch_proof = BatchDLEQProof(
        rehydrated_blinded_tokens, rehydrated_signed_tokens, this->signing_key);

    std::string base64_batch_proof = server_batch_proof.encode_base64();

    this->signed_tokens = stamped;
    this->batch_dleq_proof = base64_batch_proof;

    return;
  }

///////////////////////////////////////////////////////////////////

void ConfirmationsImpl::SetWalletInfo(std::unique_ptr<WalletInfo> info) {
  wallet_info_.payment_id = info->payment_id;
  wallet_info_.signing_key = info->signing_key;
}

void ConfirmationsImpl::SetCatalogIssuers(std::unique_ptr<IssuersInfo> info) {
  std::vector<std::string> names = {};
  std::vector<std::string> public_keys = {};

  for (const auto& issuer : info->issuers) {
    auto name = issuer.name;
    names.push_back(name);

    auto public_key = issuer.public_key;
    public_keys.push_back(public_key);
  }

  Step1StoreTheServersConfirmationsPublicKeyAndGenerator(
      info->public_key, names, public_keys);
}

void ConfirmationsImpl::AdSustained(std::unique_ptr<NotificationInfo> info) {
  Step3RedeemConfirmation(info->creative_set_id);
}

void ConfirmationsImpl::OnTimer(const uint32_t timer_id) {
  if (timer_id == step_2_refill_confirmations_timer_id_) {
    RefillConfirmations();
  } else if (timer_id == step_4_retrieve_payment_ious_timer_id_) {
    RetrievePaymentIOUS();
  } else if (timer_id == step_5_cash_in_payment_ious_timer_id_) {
    CashInPaymentIOUS();
  }
}

void ConfirmationsImpl::StartRefillingConfirmations(
    const uint64_t start_timer_in) {
  StopRefillingConfirmations();

  step_2_refill_confirmations_timer_id_ =
      confirmations_client_->SetTimer(start_timer_in);
  if (step_2_refill_confirmations_timer_id_ == 0) {
    BLOG(ERROR) << "Failed to start refilling confirmations"
        << " due to an invalid timer";

    return;
  }

  BLOG(INFO) << "Start refilling confirmations in " << start_timer_in
      << " seconds";
}

void ConfirmationsImpl::RefillConfirmations() {
  if (!is_initialized_) {
    return;
  }

  BLOG(INFO) << "Refill confirmations";

  Step2RefillConfirmationsIfNecessary(
      wallet_info_.payment_id,
      wallet_info_.signing_key,
      this->server_confirmation_key_);

  auto start_timer_in = kRefillConfirmationsAfterSeconds;
  auto rand_delay = base::RandInt(0, start_timer_in / 10);
  start_timer_in += rand_delay;

  StartRefillingConfirmations(start_timer_in);
}

void ConfirmationsImpl::StopRefillingConfirmations() {
  if (!IsRefillingConfirmations()) {
    return;
  }

  BLOG(INFO) << "Stopped refilling confirmations";

  confirmations_client_->KillTimer(step_2_refill_confirmations_timer_id_);
  step_2_refill_confirmations_timer_id_ = 0;
}

bool ConfirmationsImpl::IsRefillingConfirmations() const {
  if (step_2_refill_confirmations_timer_id_ == 0) {
    return false;
  }

  return true;
}

void ConfirmationsImpl::StartRetrievingPaymentIOUS(
    const uint64_t start_timer_in) {
  StopRetrievingPaymentIOUS();

  step_4_retrieve_payment_ious_timer_id_ =
      confirmations_client_->SetTimer(start_timer_in);
  if (step_4_retrieve_payment_ious_timer_id_ == 0) {
    BLOG(ERROR) <<
        "Failed to start retrieving payment IOUs due to an invalid timer";

    return;
  }

  BLOG(INFO) << "Start retrieving payment IOUs in " << start_timer_in
      << " seconds";
}

void ConfirmationsImpl::RetrievePaymentIOUS() {
  if (!is_initialized_) {
    return;
  }

  BLOG(INFO) << "Retrieve payment IOUs";

  Step4RetrievePaymentIOUs();

  auto start_timer_in = kRetrievePaymentIOUSAfterSeconds;
  auto rand_delay = base::RandInt(0, start_timer_in / 10);
  start_timer_in += rand_delay;

  StartRetrievingPaymentIOUS(start_timer_in);
}

void ConfirmationsImpl::StopRetrievingPaymentIOUS() {
  if (!IsRetrievingPaymentIOUS()) {
    return;
  }

  BLOG(INFO) << "Stopped retrieving payment IOUs";

  confirmations_client_->KillTimer(step_4_retrieve_payment_ious_timer_id_);
  step_4_retrieve_payment_ious_timer_id_ = 0;
}

bool ConfirmationsImpl::IsRetrievingPaymentIOUS() const {
  if (step_4_retrieve_payment_ious_timer_id_ == 0) {
    return false;
  }

  return true;
}

void ConfirmationsImpl::StartCashingInPaymentIOUS(
    const uint64_t start_timer_in) {
  StopCashingInPaymentIOUS();

  step_5_cash_in_payment_ious_timer_id_ =
      confirmations_client_->SetTimer(start_timer_in);
  if (step_5_cash_in_payment_ious_timer_id_ == 0) {
    BLOG(ERROR) <<
        "Failed to start cashing in payment IOUs due to an invalid timer";

    return;
  }

  BLOG(INFO) << "Start cashing in payment IOUs in " << start_timer_in
      << " seconds";
}

void ConfirmationsImpl::CashInPaymentIOUS() {
  if (!is_initialized_) {
    return;
  }

  BLOG(INFO) << "Cash in payment IOUs";

  Step5CashInPaymentIOUs(wallet_info_.payment_id);

  StartCashingInPaymentIOUS(kCashInPaymentIOUSAfterSeconds);
}

void ConfirmationsImpl::StopCashingInPaymentIOUS() {
  if (!IsCashingInPaymentIOUS()) {
    return;
  }

  BLOG(INFO) << "Stopped cashing in payment IOUs";

  confirmations_client_->KillTimer(step_5_cash_in_payment_ious_timer_id_);
  step_5_cash_in_payment_ious_timer_id_ = 0;
}

bool ConfirmationsImpl::IsCashingInPaymentIOUS() const {
  if (step_5_cash_in_payment_ious_timer_id_ == 0) {
    return false;
  }

  return true;
}

}  // namespace confirmations
