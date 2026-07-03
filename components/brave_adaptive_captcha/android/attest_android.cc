/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_adaptive_captcha/android/attest_android.h"

#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "base/values.h"
#include "brave/components/brave_adaptive_captcha/server_util.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "url/gurl.h"

// `VLOG` is dead-code-stripped in official Android release builds:
// base/logging.h compiles the templated `GetVlogLevel()` to a constexpr -1
// there (a binary size optimization), so every `VLOG` site — string literals
// included — is removed and never reaches logcat. This honors the
// `*/brave_adaptive_captcha/*` vmodule that the "Verbose Logs for Ad-Rewards".
#define CAPTCHA_VLOG(verbose_level)                                         \
  LAZY_STREAM(                                                              \
      ::logging::LogMessage(__FILE__, __LINE__, -(verbose_level)).stream(), \
      (verbose_level) <=                                                    \
          ::logging::GetVlogLevelHelper(__FILE__, sizeof(__FILE__)))

namespace brave_adaptive_captcha {

namespace {

constexpr char kContentType[] = "application/json";

net::NetworkTrafficAnnotationTag kAnnotationTag =
    net::DefineNetworkTrafficAnnotation("brave_adaptive_captcha_attestation",
                                        R"(
        semantics {
          sender: "Brave Rewards"
          description:
            "Performs Android device attestation to solve a scheduled Brave "
            "Rewards captcha: requests an attestation nonce, submits a Google "
            "Play Integrity token for it, then posts the captcha solution."
          trigger:
            "A captcha was scheduled for the user's Brave Rewards payment id."
          data:
            "Brave Rewards payment id, a Play Integrity token, and the app "
            "package name."
          destination: WEBSITE
        }
        policy {
          cookies_allowed: NO
          setting: "This feature cannot be disabled by settings."
          policy_exception_justification: "Not implemented."
        })");

std::string ToJson(const base::DictValue& dict) {
  std::string json;
  base::JSONWriter::Write(dict, &json);
  return json;
}

}  // namespace

AttestAndroid::AttestAndroid(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : api_request_helper_(kAnnotationTag, std::move(url_loader_factory)) {}

AttestAndroid::~AttestAndroid() = default;

void AttestAndroid::StartAttestation(const std::string& payment_id,
                                     OnStartAttestation callback) {
  const std::string url =
      ServerUtil::GetInstance()->GetServerUrl("/v1/attestations/android");
  CAPTCHA_VLOG(1) << "Start attestation: " << url;

  base::DictValue body;
  body.Set("paymentId", payment_id);

  api_request_helper_.Request(
      "POST", GURL(url), ToJson(body), kContentType,
      base::BindOnce(&AttestAndroid::OnStartAttestationResponse,
                     base::Unretained(this), std::move(callback)));
}

void AttestAndroid::OnStartAttestationResponse(
    OnStartAttestation callback,
    api_request_helper::APIRequestResult api_request_result) {
  if (api_request_result.response_code() != net::HTTP_CREATED) {
    CAPTCHA_VLOG(1) << "Start attestation failed, HTTP status: "
                    << api_request_result.response_code();
    std::move(callback).Run(std::string());
    return;
  }

  const base::Value& body = api_request_result.value_body();
  if (!body.is_dict()) {
    CAPTCHA_VLOG(1) << "Start attestation returned an invalid response";
    std::move(callback).Run(std::string());
    return;
  }

  const std::string* unique_value = body.GetDict().FindString("uniqueValue");
  if (!unique_value) {
    CAPTCHA_VLOG(1) << "Start attestation response is missing uniqueValue";
    std::move(callback).Run(std::string());
    return;
  }

  CAPTCHA_VLOG(1) << "Start attestation call is successful";
  std::move(callback).Run(*unique_value);
}

void AttestAndroid::AttestPaymentId(const std::string& payment_id,
                                    const std::string& captcha_id,
                                    const std::string& integrity_token,
                                    const std::string& unique_value,
                                    const std::string& package_name,
                                    OnAttestResult callback) {
  const std::string url = ServerUtil::GetInstance()->GetServerUrl(
      base::StrCat({"/v1/attestations/android/", payment_id}));
  CAPTCHA_VLOG(1) << "Attest payment id: " << url;

  base::DictValue body;
  body.Set("integrityToken", integrity_token);
  body.Set("uniqueValue", unique_value);
  body.Set("packageName", package_name);

  api_request_helper_.Request(
      "PUT", GURL(url), ToJson(body), kContentType,
      base::BindOnce(&AttestAndroid::OnAttestPaymentIdResponse,
                     base::Unretained(this), payment_id, captcha_id,
                     std::move(callback)));
}

void AttestAndroid::OnAttestPaymentIdResponse(
    const std::string& payment_id,
    const std::string& captcha_id,
    OnAttestResult callback,
    api_request_helper::APIRequestResult api_request_result) {
  if (api_request_result.response_code() != net::HTTP_OK) {
    CAPTCHA_VLOG(1) << "Attest payment id failed, HTTP status: "
                    << api_request_result.response_code();
    std::move(callback).Run(false);
    return;
  }

  CAPTCHA_VLOG(1) << "Attest payment id call is successful";
  SolveCaptcha(payment_id, captcha_id, std::move(callback));
}

void AttestAndroid::SolveCaptcha(const std::string& payment_id,
                                 const std::string& captcha_id,
                                 OnAttestResult callback) {
  const std::string url = ServerUtil::GetInstance()->GetServerUrl(
      absl::StrFormat("/v3/captcha/solution/%s/%s", payment_id, captcha_id));
  CAPTCHA_VLOG(1) << "Solve captcha: " << url;

  base::DictValue body;
  body.Set("solution", payment_id);

  api_request_helper_.Request(
      "POST", GURL(url), ToJson(body), kContentType,
      base::BindOnce(&AttestAndroid::OnSolveCaptchaResponse,
                     base::Unretained(this), std::move(callback)));
}

void AttestAndroid::OnSolveCaptchaResponse(
    OnAttestResult callback,
    api_request_helper::APIRequestResult api_request_result) {
  const bool success = api_request_result.response_code() == net::HTTP_OK;
  if (success) {
    CAPTCHA_VLOG(1) << "Captcha has been solved.";
  } else {
    CAPTCHA_VLOG(1) << "Captcha solution failed, HTTP status: "
                    << api_request_result.response_code();
  }
  std::move(callback).Run(success);
}

}  // namespace brave_adaptive_captcha

#undef CAPTCHA_VLOG
