// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/payments/bat_payment_app.h"

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "components/payments/core/payment_app.h"

namespace payments {
  BatPaymentApp::BatPaymentApp(): PaymentApp(0, PaymentApp::Type::SERVICE_WORKER_APP) {
      app_method_names_.insert("bat");
  }
  BatPaymentApp::~BatPaymentApp() {}

  void BatPaymentApp::InvokePaymentApp(Delegate* delegate) {}
  bool BatPaymentApp::IsCompleteForPayment() const { return true; }
  uint32_t BatPaymentApp::GetCompletenessScore() const { return 0; }
  bool BatPaymentApp::CanPreselect() const  { return false; }
  base::string16 BatPaymentApp::GetMissingInfoLabel() const  { return base::EmptyString16(); }
  bool BatPaymentApp::IsValidForCanMakePayment() const { return false; }
  void BatPaymentApp::RecordUse()  { return; }
  bool BatPaymentApp::NeedsInstallation() const { return false; }
  base::string16 BatPaymentApp::GetLabel() const {
    return base::ASCIIToUTF16("bat");
  }
  base::string16 BatPaymentApp::GetSublabel() const { return base::EmptyString16(); }
  bool BatPaymentApp::IsValidForModifier(
      const std::string& method,
      bool supported_networks_specified,
      const std::set<std::string>& supported_networks) const { return false; }
  base::WeakPtr<PaymentApp> BatPaymentApp::AsWeakPtr() { return weak_ptr_factory_.GetWeakPtr(); }
  gfx::ImageSkia BatPaymentApp::icon_image_skia() const { return icon_image_; }
  bool BatPaymentApp::HandlesShippingAddress() const { return false; }
  bool BatPaymentApp::HandlesPayerName() const { return false; }
  bool BatPaymentApp::HandlesPayerEmail() const { return false; }
  bool BatPaymentApp::HandlesPayerPhone() const { return false; }
} //namespace payments