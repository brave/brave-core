// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PAYMENTS_BAT_PAYMENT_APP_H_
#define BRAVE_COMPONENTS_PAYMENTS_BAT_PAYMENT_APP_H_

#include "components/payments/core/payment_app.h"

namespace payments {

class BatPaymentApp : public PaymentApp {
 public:
  BatPaymentApp();
  ~BatPaymentApp() override;

  void InvokePaymentApp(Delegate* delegate) override;
  bool IsCompleteForPayment() const override;
  uint32_t GetCompletenessScore() const override;
  bool CanPreselect() const override;
  base::string16 GetMissingInfoLabel() const override;
  bool IsValidForCanMakePayment() const override;
  void RecordUse() override;
  bool NeedsInstallation() const override;
  base::string16 GetLabel() const override;
  base::string16 GetSublabel() const override;
  bool IsValidForModifier(
      const std::string& method,
      bool supported_networks_specified,
      const std::set<std::string>& supported_networks) const override;
  base::WeakPtr<PaymentApp> AsWeakPtr() override;
  gfx::ImageSkia icon_image_skia() const override;
  bool HandlesShippingAddress() const override;
  bool HandlesPayerName() const override;
  bool HandlesPayerEmail() const override;
  bool HandlesPayerPhone() const override;

 private:
  gfx::ImageSkia icon_image_;

  base::WeakPtrFactory<BatPaymentApp> weak_ptr_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(BatPaymentApp);
};

}  // namespace payments

#endif  // BRAVE_COMPONENTS_PAYMENTS_BAT_PAYMENT_APP_H_