/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/attestation/attestation_desktop.h"

#include <optional>
#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"

namespace brave_rewards::internal {
namespace attestation {

AttestationDesktop::AttestationDesktop(RewardsEngineImpl& engine)
    : Attestation(engine), promotion_server_(engine) {}

AttestationDesktop::~AttestationDesktop() = default;

mojom::Result AttestationDesktop::ParseClaimSolution(
    const std::string& response,
    int* x,
    int* y,
    std::string* captcha_id) {
  DCHECK(x && y && captcha_id);

  std::optional<base::Value> value = base::JSONReader::Read(response);
  if (!value || !value->is_dict()) {
    return mojom::Result::FAILED;
  }

  const base::Value::Dict& dict = value->GetDict();
  const auto* id = dict.FindString("captchaId");
  if (!id) {
    engine_->LogError(FROM_HERE) << "Captcha id is wrong";
    return mojom::Result::FAILED;
  }

  const auto x_parse = dict.FindInt("x");
  if (!x_parse) {
    engine_->LogError(FROM_HERE) << "X is wrong";
    return mojom::Result::FAILED;
  }

  const auto y_parse = dict.FindInt("y");
  if (!y_parse) {
    engine_->LogError(FROM_HERE) << "Y is wrong";
    return mojom::Result::FAILED;
  }

  *x = *x_parse;
  *y = *y_parse;
  *captcha_id = *id;
  return mojom::Result::OK;
}

void AttestationDesktop::Start(const std::string& payload,
                               StartCallback callback) {
  auto url_callback =
      base::BindOnce(&AttestationDesktop::DownloadCaptchaImage,
                     base::Unretained(this), std::move(callback));

  promotion_server_.post_captcha().Request(std::move(url_callback));
}

void AttestationDesktop::DownloadCaptchaImage(StartCallback callback,
                                              mojom::Result result,
                                              const std::string& hint,
                                              const std::string& captcha_id) {
  if (result != mojom::Result::OK) {
    std::move(callback).Run(mojom::Result::FAILED, "");
    return;
  }

  auto url_callback = base::BindOnce(
      &AttestationDesktop::OnDownloadCaptchaImage, base::Unretained(this),
      std::move(callback), hint, captcha_id);

  promotion_server_.get_captcha().Request(captcha_id, std::move(url_callback));
}

void AttestationDesktop::OnDownloadCaptchaImage(StartCallback callback,
                                                const std::string& hint,
                                                const std::string& captcha_id,
                                                mojom::Result result,
                                                const std::string& image) {
  if (result != mojom::Result::OK) {
    std::move(callback).Run(mojom::Result::FAILED, "");
    return;
  }

  base::Value::Dict dict;
  dict.Set("hint", hint);
  dict.Set("captchaId", captcha_id);
  dict.Set("captchaImage", image);

  std::string json;
  base::JSONWriter::Write(dict, &json);
  std::move(callback).Run(mojom::Result::OK, json);
}

void AttestationDesktop::Confirm(const std::string& solution,
                                 ConfirmCallback callback) {
  int x;
  int y;
  std::string captcha_id;
  const mojom::Result result =
      ParseClaimSolution(solution, &x, &y, &captcha_id);

  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Failed to parse solution";
    std::move(callback).Run(result);
    return;
  }

  auto url_callback =
      base::BindOnce(&AttestationDesktop::OnConfirm, base::Unretained(this),
                     std::move(callback));

  promotion_server_.put_captcha().Request(x, y, captcha_id,
                                          std::move(url_callback));
}

void AttestationDesktop::OnConfirm(ConfirmCallback callback,
                                   mojom::Result result) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Failed to confirm attestation";
    std::move(callback).Run(result);
    return;
  }

  std::move(callback).Run(mojom::Result::OK);
}

}  // namespace attestation
}  // namespace brave_rewards::internal
