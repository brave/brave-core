/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ATTESTATION_ATTESTATION_DESKTOP_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ATTESTATION_ATTESTATION_DESKTOP_H_

#include <memory>
#include <string>

#include "base/values.h"
#include "bat/ledger/internal/attestation/attestation.h"
#include "bat/ledger/internal/endpoint/promotion/promotion_server.h"

namespace ledger {
class LedgerImpl;

namespace attestation {

class AttestationDesktop : public Attestation {
 public:
  explicit AttestationDesktop(LedgerImpl* ledger);
  ~AttestationDesktop() override;

  void Start(const std::string& payload, StartCallback callback) override;

  void Confirm(
      const std::string& solution,
      ConfirmCallback callback) override;

 private:
  mojom::Result ParseClaimSolution(const std::string& response,
                                   int* x,
                                   int* y,
                                   std::string* captcha_id);

  void DownloadCaptchaImage(StartCallback callback,
                            mojom::Result result,
                            const std::string& hint,
                            const std::string& captcha_id);

  void OnDownloadCaptchaImage(StartCallback callback,
                              const std::string& hint,
                              const std::string& captcha_id,
                              mojom::Result result,
                              const std::string& image);

  void OnConfirm(ConfirmCallback callback, mojom::Result result);

  std::unique_ptr<endpoint::PromotionServer> promotion_server_;
};

}  // namespace attestation
}  // namespace ledger
#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ATTESTATION_ATTESTATION_DESKTOP_H_
