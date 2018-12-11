/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "confirmations_impl.h"
#include "bat/confirmations/confirmations_client.h"
#include "bat/confirmations/catalog_issuers_info.h"
#include "logging.h"
#include "static_values.h"

#include <vector>
#include <iostream>
#include <memory>

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

extern int count;
extern std::string happy_data; 
extern int happy_status;

void OnBegin( const happyhttp::Response* r, void* userdata )
{
  // printf("BEGIN (%d %s)\n", r->getstatus(), r->getreason() );
  count = 0;
  happy_data = "";
  happy_status = r->getstatus();
}

void OnData( const happyhttp::Response* r, void* userdata, const unsigned char* data, int n )
{
  //fwrite( data,1,n, stdout );
  happy_data.append((char *)data, (size_t)n);
  count += n;
}

void OnComplete( const happyhttp::Response* r, void* userdata )
{
  happy_status = r->getstatus();
  // printf("END (%d %s)\n", r->getstatus(), r->getreason() );
  // printf("COMPLETE (%d bytes)\n", count );
}

namespace confirmations {

  void vector_concat(std::vector<std::string> &dest, std::vector<std::string> &source) {
    dest.insert(dest.end(), source.begin(), source.end());
  }

  std::unique_ptr<base::ListValue> munge(std::vector<std::string> v) {

    base::ListValue * list = new base::ListValue();

    for (auto x : v) {
      list->AppendString(x);
    }

    return std::unique_ptr<base::ListValue>(list); 
  }

  std::vector<std::string> ConfirmationsImpl::unmunge(base::Value *value) {
    std::vector<std::string> v;

    base::ListValue list(value->GetList());

    for (size_t i = 0; i < list.GetSize(); i++) {
      base::Value *x;
      list.Get(i, &x);
      v.push_back(x->GetString());
    }

    return v;
  }


  bool ConfirmationsImpl::confirmations_ready_for_ad_showing() {

    // TODO Whatever thread/service calls this in brave-core-client must also 
    //      be the one that triggers ad showing, or we'll have a race condition
    // mutex.lock();
    bool ready = (signed_blinded_confirmation_tokens.size() > 0);
    // mutex.unlock();

    return ready;
  }

  void ConfirmationsImpl::step_1_storeTheServersConfirmationsPublicKeyAndGenerator(std::string confirmations_GH_pair, std::string payments_GH_pair, std::vector<std::string> bat_names, std::vector<std::string> bat_keys) {
    // This (G,H) *pair* is exposed as a *single* string via the rust lib
    // G is the generator the server used in H, see next line
    // H, aka Y, is xG, the server's public key
    // These are both necessary for the DLEQ proof, but not useful elsewhere
    // These come back with the catalog from the server
    // Later we'll get an *array of pairs* for the payments side
    this->server_confirmation_key = confirmations_GH_pair;
    this->server_payment_key = payments_GH_pair;
    this->server_bat_payment_names = bat_names;
    this->server_bat_payment_keys = bat_keys;
  
    // for(auto x : bat_names) { BLOG(ERROR) << "x: " << (x) << "\n"; }
  
    this->saveState();
    BLOG(INFO) << "step1.1 : key: " << this->server_confirmation_key << std::endl;
  }

  std::string ConfirmationsImpl::toJSONString() {
    base::DictionaryValue dict;
     
    dict.SetKey("issuers_version", base::Value(issuers_version));
    dict.SetKey("server_confirmation_key", base::Value(server_confirmation_key));
    dict.SetKey("server_payment_key", base::Value(server_payment_key));
    dict.SetWithoutPathExpansion("server_bat_payment_names", munge(server_bat_payment_names));
    dict.SetWithoutPathExpansion("server_bat_payment_keys", munge(server_bat_payment_keys));
    dict.SetWithoutPathExpansion("original_confirmation_tokens", munge(original_confirmation_tokens));
    dict.SetWithoutPathExpansion("blinded_confirmation_tokens", munge(blinded_confirmation_tokens));
    dict.SetWithoutPathExpansion("signed_blinded_confirmation_tokens", munge(signed_blinded_confirmation_tokens));
    dict.SetWithoutPathExpansion("payment_token_json_bundles", munge(payment_token_json_bundles));
    dict.SetWithoutPathExpansion("signed_blinded_payment_token_json_bundles", munge(signed_blinded_payment_token_json_bundles));
    dict.SetWithoutPathExpansion("fully_submitted_payment_bundles", munge(fully_submitted_payment_bundles));

    std::string json;
    base::JSONWriter::Write(dict, &json);

    //std::unique_ptr<base::Value> val( base::JSONReader::Read(json) );
    //assert(dict.Equals(val.get()));

    return json;
  }


  void ConfirmationsImpl::step_2_refillConfirmationsIfNecessary(std::string real_wallet_address,
                                                            std::string real_wallet_address_secret_key,
                                                            std::string local_server_confirmation_key) {

    if (this->blinded_confirmation_tokens.size() > low_token_threshold) {
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
  
    BLOG(INFO) << "step2.1 : batch generate, count: " << local_original_confirmation_tokens.size() << std::endl;

    {
      std::string digest = "digest";
      std::string primary = "primary";
      
      /////////////////////////////////////////////////////////////////////////////
      std::string build = "";
      
      build.append("{\"blindedTokens\":");
      build.append("[");
      std::vector<std::string> a = local_blinded_confirmation_tokens;
      
      for(size_t i = 0; i < a.size(); i++) {
      if(i > 0) {
        build.append(",");
      }
      build.append("\"");
      build.append(a[i]);
      build.append("\"");
      }
      
      build.append("]");
      build.append("}");
      
      std::string real_body = build;
      
      std::vector<uint8_t> real_sha_raw = this->getSHA256(real_body);
      std::string real_body_sha_256_b64 = this->getBase64(real_sha_raw);
      
      std::vector<uint8_t> real_skey = this->rawDataBytesVectorFromASCIIHexString(real_wallet_address_secret_key);
      
      std::string real_digest_field = std::string("SHA-256=").append(real_body_sha_256_b64);

      std::string real_signature_field = this->sign(&digest, &real_digest_field, 1, primary, real_skey);
      /////////////////////////////////////////////////////////////////////////////

      happyhttp::Connection conn(BRAVE_AD_SERVER, BRAVE_AD_SERVER_PORT);
      conn.setcallbacks( OnBegin, OnData, OnComplete, 0 );

      // step 2.2 POST /v1/confirmation/token/{payment_id}

      BLOG(INFO) << "step2.2 : POST /v1/confirmation/token/{payment_id}: " << real_wallet_address << std::endl;
      std::string endpoint = std::string("/v1/confirmation/token/").append(real_wallet_address);
 
      const char * h[] = {"digest", (const char *) real_digest_field.c_str(), 
                          "signature", (const char *) real_signature_field.c_str(), 
                          "accept", "application/json",
                          "Content-Type", "application/json",
                          NULL, NULL };
 
      conn.request("POST", endpoint.c_str(), h, (const unsigned char *)real_body.c_str(), real_body.size());
 
      while( conn.outstanding() ) conn.pump();
 
      std::string post_resp = happy_data;
 
      //TODO this should be the `nonce` in the return. we need to make sure we get the nonce in the separate request  
      //observation. seems like we should move all of this (the tokens in-progress) data to a map keyed on the nonce, and then
      //step the storage through (pump) in a state-wise (dfa) as well, so the storage types are coded (named) on a dfa-state-respecting basis
 
      // TODO 2.2 POST: on inet failure, retry or cleanup & unlock
      
      std::unique_ptr<base::Value> value(base::JSONReader::Read(post_resp));
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
 
      // TODO Instead of pursuing true asynchronicity at this point, what we can do is sleep for a minute or two
      //      and blow away any work to this point on failure
      //      this solves the problem for now since the tokens have no value at this point
 
      //STEP 2.3
      // TODO this is done blocking and assumes success but we need to separate it more and account for the possibility of failures
      // TODO GET: on inet failure, retry or cleanup & unlock
      { 
        BLOG(INFO) << "step2.3 : GET  /v1/confirmation/token/{payment_id}?nonce=: " << nonce << std::endl;
        happyhttp::Connection conn(BRAVE_AD_SERVER, BRAVE_AD_SERVER_PORT);
        conn.setcallbacks( OnBegin, OnData, OnComplete, 0 );
 
        // /v1/confirmation/token/{payment_id}
        std::string endpoint = std::string("/v1/confirmation/token/").append(real_wallet_address).append("?nonce=").append(nonce);

        conn.request("GET", endpoint.c_str()  ); // h, (const unsigned char *)real_body.c_str(), real_body.size());

        while( conn.outstanding() ) conn.pump();
        std::string get_resp = happy_data;

        /////////////////////////////////////////////////////////

        // happy_data: {"batchProof":"r2qx2h5ENHASgBxEhN2TjUjtC2L2McDN6g/lZ+nTaQ6q+6TZH0InhxRHIp0vdUlSbMMCHaPdLYsj/IJbseAtCw==","signedTokens":["VI27MCax4V9Gk60uC1dwCHHExHN2WbPwwlJk87fYAyo=","mhFmcWHLk5X8v+a/X0aea24OfGWsfAwWbP7RAeXXLV4="]}

        std::unique_ptr<base::Value> value(base::JSONReader::Read(get_resp));
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

        bool real_verified = this->verifyBatchDLEQProof(real_batch_proof,
                                                        local_blinded_confirmation_tokens,
                                                        server_signed_blinded_confirmations,
                                                        local_server_confirmation_key);
        if (!real_verified) {
          // 2018.11.29 kevin - ok to log these only (maybe forever) but don't consider failing until after we're versioned on "issuers" private keys 
          // 2018.12.10 actually, ok to fail on the confirmations portion of this now. we don't want bad tokens anyway
          BLOG(ERROR) << "ERROR: Server confirmations proof invalid" << std::endl;
          return;
        }

        {
          //finally, if everything succeeded we'll modify object state and persist
          BLOG(INFO) << "step2.4 : store the signed blinded confirmations tokens & pre data" << std::endl;
          vector_concat(this->original_confirmation_tokens, local_original_confirmation_tokens);
          vector_concat(this->blinded_confirmation_tokens, local_blinded_confirmation_tokens);
          vector_concat(this->signed_blinded_confirmation_tokens, server_signed_blinded_confirmations);
          this->saveState();
        }

      } // 2.3

     } // 2.1
  }

  void ConfirmationsImpl::step_3_redeemConfirmation(std::string real_creative_instance_id) {

    if (this->signed_blinded_confirmation_tokens.size() <= 0) {
      BLOG(INFO) << "ERROR: step 3.1a, no signed blinded confirmation tokens" << std::endl;
      return;
    }

    BLOG(INFO) << "step3.1a: unblinding signed blinded confirmations" << std::endl;

    std::string orig_token_b64 = this->original_confirmation_tokens.front();
    std::string sb_token_b64 = this->signed_blinded_confirmation_tokens.front();

    // rehydrate
    Token restored_token = Token::decode_base64(orig_token_b64);
    SignedToken signed_token = SignedToken::decode_base64(sb_token_b64);
    // use blinding scalar to unblind
    UnblindedToken client_unblinded_token = restored_token.unblind(signed_token);
    // dehydrate  
    std::string base64_unblinded_token = client_unblinded_token.encode_base64();

    std::string local_unblinded_signed_confirmation_token = base64_unblinded_token;

    // we're doing this here instead of doing it on success and tracking success/failure
    // since it's cheaper development wise. but optimization wise, it "wastes" a (free) confirmation
    // token on failure
    this->popFrontConfirmation();
    // persist
    this->saveState();

    BLOG(INFO) << "step3.1b: generate payment, count: " << original_confirmation_tokens.size() << std::endl;

    // client prepares a random token and blinding scalar pair
    Token token = Token::random();
    std::string token_base64 = token.encode_base64();

    // client blinds the token
    BlindedToken blinded_token = token.blind();
    std::string blinded_token_base64 = blinded_token.encode_base64();

    std::string local_original_payment_token = token_base64;
    std::string local_blinded_payment_token = blinded_token_base64;

    // // what's `t`? local_unblinded_signed_confirmation_token
    // // what's `MAC_{sk}(R)`? item from blinded_payment_tokens

    // std::string prePaymentToken = local_blinded_payment_token; 
    std::string blindedPaymentToken = local_blinded_payment_token; 
    std::string json;
    
    // build body of POST request
    base::DictionaryValue dict;
    dict.SetKey("creativeInstanceId", base::Value(real_creative_instance_id));
    dict.SetKey("payload", base::Value(base::Value::Type::DICTIONARY));
    //dict.SetKey("prePaymentToken", base::Value(prePaymentToken));
    dict.SetKey("blindedPaymentToken", base::Value(blindedPaymentToken));
    dict.SetKey("type", base::Value("landed"));
    base::JSONWriter::Write(dict, &json);

    UnblindedToken restored_unblinded_token = UnblindedToken::decode_base64(local_unblinded_signed_confirmation_token);
    VerificationKey client_vKey = restored_unblinded_token.derive_verification_key();
    std::string message = json;
    VerificationSignature client_sig = client_vKey.sign(message);

    std::string base64_token_preimage = restored_unblinded_token.preimage().encode_base64();
    std::string base64_signature = client_sig.encode_base64();

    base::DictionaryValue bundle;
    std::string credential_json;
    bundle.SetKey("payload", base::Value(json));
    bundle.SetKey("signature", base::Value(base64_signature));
    bundle.SetKey("t", base::Value(base64_token_preimage));
    base::JSONWriter::Write(bundle, &credential_json);

    std::vector<uint8_t> vec(credential_json.begin(), credential_json.end());
    std::string b64_encoded_a = this->getBase64(vec);

    std::string b64_encoded;
    base::Base64Encode(credential_json, &b64_encoded);

    DCHECK(b64_encoded_a == b64_encoded);

    std::string uri_encoded = net::EscapeQueryParamValue(b64_encoded, true);

    // 3 pieces we need for our POST request, 1 for URL, 1 for body, and 1 for URL that depends on body
    std::string confirmation_id = base::GenerateGUID();
    std::string real_body = json;
    std::string credential = uri_encoded;

    ///////////////////////////////////////////////////////////////////////
    // step_3_1c POST /v1/confirmation/{confirmation_id}/{credential}, which is (t, MAC_(sk)(R))
    BLOG(INFO) << "step3.1c: POST /v1/confirmation/{confirmation_id}/{credential} " << confirmation_id << std::endl;
    happyhttp::Connection conn(BRAVE_AD_SERVER, BRAVE_AD_SERVER_PORT);
    conn.setcallbacks( OnBegin, OnData, OnComplete, 0 );

    std::string endpoint = std::string("/v1/confirmation/").append(confirmation_id).append("/").append(credential);
    
    // -d "{ \"creativeInstanceId\": \"6ca04e53-2741-4d62-acbb-e63336d7ed46\", \"payload\": {}, \"prePaymentToken\": \"cgILwnP8ua+cZ+YHJUBq4h+U+mt6ip8lX9hzElHrSBg=\", \"type\": \"landed\" }"
    const char * h[] = {"accept", "application/json",
                        "Content-Type", "application/json",
                        NULL, NULL };

    conn.request("POST", endpoint.c_str(), h, (const unsigned char *)real_body.c_str(), real_body.size());

    while( conn.outstanding() ) conn.pump();
    std::string post_resp = happy_data;
    ///////////////////////////////////////////////////////////////////////

    if (happy_status == 201) {  // 201 - created
      std::unique_ptr<base::Value> value(base::JSONReader::Read(happy_data));
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

      //check return code, check json for `id` key
      //for bundle:
      //  ✓ confirmation_id
      //  ✓ local_original_payment_token
      //  ✓ local_blinded_payment_token - we do need: for DLEQ proof
      //  ✗ bundle_timestamp - nice to have in case we want to expire later

      std::string timestamp = std::to_string(base::Time::NowFromSystemTime().ToTimeT());

      BLOG(INFO) << "step3.2 : store confirmationId &such" << std::endl;
      base::DictionaryValue bundle;
      bundle.SetKey("confirmation_id", base::Value(confirmation_id));
      bundle.SetKey("original_payment_token", base::Value(local_original_payment_token));
      bundle.SetKey("blinded_payment_token", base::Value(local_blinded_payment_token));
      bundle.SetKey("bundle_timestamp", base::Value(timestamp));

      std::string bundle_json;
      base::JSONWriter::Write(bundle, &bundle_json);
      this->payment_token_json_bundles.push_back(bundle_json);
      this->saveState();
    }
  }

  bool ConfirmationsImpl::processIOUBundle(std::string bundle_json) {

    bool unfinished = false;
    bool finished   = true;

    std::string confirmation_id;
    std::string original_payment_token;
    std::string blinded_payment_token;

    std::unique_ptr<base::Value> bundle_value(base::JSONReader::Read(bundle_json));
    base::DictionaryValue* map;
    if (!bundle_value->GetAsDictionary(&map)) {
      BLOG(ERROR) << "no 4 process iou bundle dict" << "\n";
      return finished;
    }

    base::Value *u;

    if (!(u = map->FindKey("confirmation_id"))) {
      BLOG(ERROR) << "4 process iou bundle, could not get confirmation_id\n";
      return finished;
    }
    confirmation_id = u->GetString();

    if (!(u = map->FindKey("original_payment_token"))) {
      BLOG(ERROR) << "4 process iou bundle, could not get original_payment_token\n";
      return finished;
    }
    original_payment_token = u->GetString();
 
    if (!(u = map->FindKey("blinded_payment_token"))) {
      BLOG(ERROR) << "4 process iou bundle, could not get blinded_payment_token\n";
      return finished;
    }
    blinded_payment_token = u->GetString();
  
    // 4.1 GET /v1/confirmation/{confirmation_id}/paymentToken
    BLOG(INFO) << "step4.1 : GET /v1/confirmation/{confirmation_id}/paymentToken" << std::endl;
 
    happyhttp::Connection conn(BRAVE_AD_SERVER, BRAVE_AD_SERVER_PORT);
    conn.setcallbacks( OnBegin, OnData, OnComplete, 0 );
 
    std::string endpoint = std::string("/v1/confirmation/").append(confirmation_id).append("/paymentToken");
     
    conn.request("GET", endpoint.c_str());
 
    while( conn.outstanding() ) conn.pump();
 
    int get_resp_code = happy_status;
    std::string get_resp = happy_data;

    if (!(get_resp_code == 200 || get_resp_code == 202))  { 
      // something broke before server could decide paid:true/false
      // TODO inet failure: retry or cleanup & unlock
      return unfinished;
    }

    // 2018.12.10 apparently, server side has changed to always pay tokens, so we won't recv 202 response?
    if (get_resp_code == 202) { // paid:false response
      // 1. collect estimateToken from JSON
      // 2. derive estimate

      std::unique_ptr<base::Value> value(base::JSONReader::Read(get_resp));
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
 
    if (get_resp_code == 200) { // paid:true response
      base::Value *v;
      std::unique_ptr<base::Value> value(base::JSONReader::Read(get_resp));
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
        BLOG(ERROR) << "4.1 200 currently unsupported size for signedTokens array\n";
        return unfinished;
      }

      for (size_t i = 0; i < signedTokensList.GetSize(); i++) {
        base::Value *x;
        signedTokensList.Get(i, &x);
        signedBlindedTokens.push_back(x->GetString());
      }

      std::vector<std::string> local_blinded_payment_tokens = {blinded_payment_token};

      bool real_verified = this->verifyBatchDLEQProof(batchProof, 
                                                      local_blinded_payment_tokens,
                                                      signedBlindedTokens,
                                                      publicKey);
      if (!real_verified) {
        //2018.11.29 kevin - ok to log these only (maybe forever) but don't consider failing until after we're versioned on "issuers" private keys 
        BLOG(ERROR) << "ERROR: Real payment proof invalid" << std::endl;
      }

      std::string name = this->BATNameFromBATPublicKey(publicKey);
      std::string payment_worth = "";
      if (name != "") {
        payment_worth = name;
      } else {
        BLOG(ERROR) << "Step 4.1/4.2 200 verification empty name \n";
      }

      for (auto signedBlindedPaymentToken : signedBlindedTokens) {
        BLOG(INFO) << "step4.2 : store signed blinded payment token" << std::endl;
        map->SetKey("signed_blinded_payment_token", base::Value(signedBlindedPaymentToken));
        map->SetKey("server_payment_key", base::Value(publicKey));
        map->SetKey("payment_worth", base::Value(payment_worth));

        std::string json_with_signed;
        base::JSONWriter::Write(*map, &json_with_signed);

        this->signed_blinded_payment_token_json_bundles.push_back(json_with_signed);
        this->saveState();
      }

      return finished;
    } 
   
    return unfinished;
  }

  void ConfirmationsImpl::step_4_retrievePaymentIOUs() {
    // we cycle through this multiple times until the token is marked paid
    std::vector<std::string> remain = {};
    for (auto payment_bundle_json : this->payment_token_json_bundles) {
      bool finished = processIOUBundle(payment_bundle_json);
      if (!finished) {
          remain.push_back(payment_bundle_json);
      }
    }
    this->payment_token_json_bundles = remain;
    this->saveState();
  }

  void ConfirmationsImpl::step_5_cashInPaymentIOUs(std::string real_wallet_address) {

    BLOG(INFO) << "step5.1 : unblind signed blinded payments" << std::endl;

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
        BLOG(ERROR) << "5 process iou bundle, could not get server_payment_key\n";
        return;
      }
      std::string server_payment_key = u->GetString();
      
      if (!(u = map->FindKey("original_payment_token"))) {
        BLOG(ERROR) << "5 process iou bundle, could not get original_payment_token\n";
        return;
      }
      std::string original_payment_token = u->GetString();
 
      if (!(u = map->FindKey("signed_blinded_payment_token"))) {
        BLOG(ERROR) << "5 process iou bundle, could not get signed_blinded_payment_token\n";
        return;
      }
      std::string signed_blinded_payment_token = u->GetString();
 
      std::string orig_token_b64 = original_payment_token;
      std::string sb_token_b64 = signed_blinded_payment_token;

      // rehydrate
      Token restored_token = Token::decode_base64(orig_token_b64);
      SignedToken signed_token = SignedToken::decode_base64(sb_token_b64);
      // use blinding scalar to unblind
      UnblindedToken client_unblinded_token = restored_token.unblind(signed_token);
      // dehydrate  
      std::string base64_unblinded_token = client_unblinded_token.encode_base64();
      // put on object
      local_unblinded_signed_payment_tokens.push_back(base64_unblinded_token);
      local_payment_keys.push_back(server_payment_key);
    }

    // PUT /v1/confirmation/token/{payment_id}
    std::string endpoint = std::string("/v1/confirmation/payment/").append(real_wallet_address);

    //{}->payload->{}->payment_id                               real_wallet_address
    //{}->paymentCredentials->[]->{}->credential->{}->signature signature of payload
    //{}->paymentCredentials->[]->{}->credential->{}->t         uspt
    //{}->paymentCredentials->[]->{}->publicKey                 server_payment_key

    std::string primary = "primary";
    std::string pay_key = "paymentId";
    std::string pay_val = real_wallet_address;

    base::DictionaryValue payload;
    payload.SetKey(pay_key, base::Value(pay_val));

    std::string payload_json;
    base::JSONWriter::Write(payload, &payload_json);

    base::ListValue * list = new base::ListValue();

    // TODO have brianjohnson/nejc/terry spot check this block to make sure new/::move/::unique_ptr usage is right
    for (size_t i = 0; i < local_unblinded_signed_payment_tokens.size(); i++) {

      auto uspt = local_unblinded_signed_payment_tokens[i];
      std::string server_payment_key = local_payment_keys[i];

      UnblindedToken restored_unblinded_token = UnblindedToken::decode_base64(uspt);
      VerificationKey client_vKey = restored_unblinded_token.derive_verification_key();
      std::string message = payload_json;
      VerificationSignature client_sig = client_vKey.sign(message);
      std::string base64_signature = client_sig.encode_base64();
      std::string base64_token_preimage = restored_unblinded_token.preimage().encode_base64();

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
    sdict.SetWithoutPathExpansion("paymentCredentials", std::unique_ptr<base::ListValue>(list));
    //sdict.SetKey("payload", std::move(payload));
    sdict.SetKey("payload", base::Value(payload_json));

    std::string json;
    base::JSONWriter::Write(sdict, &json);

    const char * h[] = { "accept", "application/json",
                         "Content-Type", "application/json",
                         NULL, NULL };

    std::string real_body = json;

    happyhttp::Connection conn(BRAVE_AD_SERVER, BRAVE_AD_SERVER_PORT);
    conn.setcallbacks( OnBegin, OnData, OnComplete, 0 );
    conn.request("PUT", endpoint.c_str(), h, (const unsigned char *)real_body.c_str(), real_body.size());

    while( conn.outstanding() ) conn.pump();
    std::string put_resp = happy_data;
    int put_resp_code = happy_status;

    if (put_resp_code != 200) {
      // TODO on inet failure, retry or cleanup & unlock ?
      return;
    }

    if (put_resp_code == 200) {

      // 2018.12.11 a 200 response from the server now means that we're done
      //            clean up state (remove succeeded bundles) and go home
      BLOG(INFO) << "step5.2 : store txn ids and actual payment" << std::endl;

      // TODO prune? (grows in # of ads watched & paid)
      vector_concat(this->fully_submitted_payment_bundles, this->signed_blinded_payment_token_json_bundles);
      this->signed_blinded_payment_token_json_bundles.clear();
      this->saveState();
      
      return;

      // NB. this still has the potential to carry an error key? 2018.12.10 is this still true anymore?

      std::unique_ptr<base::Value> value(base::JSONReader::Read(put_resp));

      base::ListValue *list;
      if (!value->GetAsList(&list)) {
        BLOG(ERROR) << "no list" << "\n";
        abort();
      }

      for (size_t i = 0; i < list->GetSize(); i++) {
        base::Value *x;
        list->Get(i, &x);

        base::DictionaryValue* dict;
        if (!x->GetAsDictionary(&dict)) {
          BLOG(ERROR) << "no dict" << "\n";
          abort();
        }

        if ((x = dict->FindKey("error"))) {
          //error case 
          std::string err = x->GetString();
          BLOG(ERROR) << "PUT error: " << err << "\n";
        } else { 
          // no error

          std::string transaction_id;

          if ((x = dict->FindKey("id"))) {
            transaction_id = x->GetString();
          } else {
            BLOG(ERROR) << "5.1 no txn id" << "\n";
            abort();
          }

        }

      }
    }

  }

  bool ConfirmationsImpl::verifyBatchDLEQProof(std::string proof_string,
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

  void ConfirmationsImpl::popFrontConfirmation() {
    auto &a = this->original_confirmation_tokens;
    auto &b = this->blinded_confirmation_tokens;
    auto &c = this->signed_blinded_confirmation_tokens;

    a.erase(a.begin());
    b.erase(b.begin());
    c.erase(c.begin());
  }

  void ConfirmationsImpl::popFrontPayment() {
    auto &a = this->signed_blinded_payment_token_json_bundles;

    a.erase(a.begin());
  }

  std::string ConfirmationsImpl::BATNameFromBATPublicKey(std::string token) {
    std::vector<std::string> &k = this->server_bat_payment_keys;

    // find position of public key in the BAT array  (later use same pos to find the `name`)
    ptrdiff_t pos = distance(k.begin(), find(k.begin(), k.end(), token));

    bool found = pos < (ptrdiff_t)k.size();

    if (!found) {
      return "";
    }

    std::string name = this->server_bat_payment_names[pos];
    return name;
  }

  std::string ConfirmationsImpl::sign(std::string* keys, std::string* values, const unsigned int& size,
       const std::string& keyId, const std::vector<uint8_t>& secretKey) {
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
     crypto_sign(&signedMsg.front(), &signedMsgSize, (const unsigned char*)message.c_str(), (unsigned long long)message.length(), &secretKey.front());

     std::vector<uint8_t> signature(crypto_sign_BYTES);
     std::copy(signedMsg.begin(), signedMsg.begin() + crypto_sign_BYTES, signature.begin());

     return "keyId=\"" + keyId + "\",algorithm=\"" + CONFIRMATIONS_SIGNATURE_ALGORITHM +
       "\",headers=\"" + headers + "\",signature=\"" + getBase64(signature) + "\"";
  }


  std::vector<uint8_t> ConfirmationsImpl::getSHA256(const std::string& in) {
    std::vector<uint8_t> res(SHA256_DIGEST_LENGTH);
    SHA256((uint8_t*)in.c_str(), in.length(), &res.front());
    return res;
  }

  std::string ConfirmationsImpl::getBase64(const std::vector<uint8_t>& in) {
    std::string res;
    size_t size = 0;
    if (!EVP_EncodedLength(&size, in.size())) {
      DCHECK(false);
      LOG(ERROR) << "EVP_EncodedLength failure in getBase64";

      return "";
    }
    std::vector<uint8_t> out(size);
    int numEncBytes = EVP_EncodeBlock(&out.front(), &in.front(), in.size());
    DCHECK(numEncBytes != 0);
    res = (char*)&out.front();
    return res;
  }

  std::vector<uint8_t> ConfirmationsImpl::rawDataBytesVectorFromASCIIHexString(std::string ascii) {
    std::vector<uint8_t> bytes;
    size_t len = ascii.length();
    for(size_t i = 0; i < len; i += 2) {
        std::string b =  ascii.substr(i, 2);
        uint8_t x = std::strtol(b.c_str(),0,16);
        bytes.push_back(x);
    }
    return bytes;
  }








  bool ConfirmationsImpl::fromJSONString(std::string json_string) {
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
    //BLOG(ERROR) << "v: " << v->GetString() << "\n";

    // if (!(v = dict->FindKey(""))) return fail;
    // this->= v->GetString();

    if (!(v = dict->FindKey("issuers_version"))) return fail;
    this->issuers_version = v->GetString();

    if (!(v = dict->FindKey("server_confirmation_key"))) return fail;
    this->server_confirmation_key = v->GetString();

    if (!(v = dict->FindKey("server_payment_key"))) return fail;
    this->server_payment_key = v->GetString();

    if (!(v = dict->FindKey("server_bat_payment_names"))) return fail;
    this->server_bat_payment_names = unmunge(v);

    if (!(v = dict->FindKey("server_bat_payment_keys"))) return fail;
    this->server_bat_payment_keys = unmunge(v);

    if (!(v = dict->FindKey("original_confirmation_tokens"))) return fail;
    this->original_confirmation_tokens = unmunge(v);

    if (!(v = dict->FindKey("blinded_confirmation_tokens"))) return fail;
    this->blinded_confirmation_tokens = unmunge(v);

    if (!(v = dict->FindKey("signed_blinded_confirmation_tokens"))) return fail;
    this->signed_blinded_confirmation_tokens = unmunge(v);

    if (!(v = dict->FindKey("signed_blinded_payment_token_json_bundles"))) return fail;
    this->signed_blinded_payment_token_json_bundles = unmunge(v);

    if (!(v = dict->FindKey("fully_submitted_payment_bundles"))) return fail;
    this->fully_submitted_payment_bundles = unmunge(v);

    return succeed;
  }

  void ConfirmationsImpl::saveState() {
    // TODO: call out to client
    std::string json = toJSONString();

    assert(this->fromJSONString(json));
    //this->server_confirmation_key = "bort";
    std::string json2 = toJSONString();
    assert(json2 == json);

    // BLOG(INFO) << json<< "\n\n\n\n";
    // BLOG(INFO) << "saving state... | ";
  }

  bool ConfirmationsImpl::loadState(std::string json_state) {
    // returns false on failure to load (eg, malformed json)
    // TODO: call out to client?

    bool fail = 0;
    bool succeed = 1;

    bool parsed = this->fromJSONString(json_state);

    if (!parsed) {
      return fail;
    }

    BLOG(INFO) << "loading state... | ";

    return succeed;
  }

///////////////////////////////////////////////////////////////////

ConfirmationsImpl::ConfirmationsImpl(
      ConfirmationsClient* confirmations_client) :
    is_initialized_(false),
    confirmations_client_(confirmations_client) {
}

ConfirmationsImpl::~ConfirmationsImpl() = default;

void ConfirmationsImpl::Initialize() {
  if (is_initialized_) {
    BLOG(WARNING) << "Already initialized";
    return;
  }

  is_initialized_ = true;

  BLOG(INFO) << BRAVE_AD_SERVER;
  BLOG(INFO) << BRAVE_AD_SERVER_PORT;
  BLOG(INFO) << "Hello";
}

void ConfirmationsImpl::OnCatalogIssuersChanged(
    const CatalogIssuersInfo& info) {
}

void ConfirmationsImpl::OnTimer(const uint32_t timer_id) {
}

}  // namespace confirmations
