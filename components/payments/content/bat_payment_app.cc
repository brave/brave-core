/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/json/json_writer.h"
#include "base/values.h"
#include "brave/components/payments/content/bat_payment_app.h"

namespace payments {

const char kBatPaymentMethod[] = "https://batpay.brave.com";
const auto kBatPaymentAppType = PaymentApp::Type::NATIVE_MOBILE_APP;

BatPaymentApp::BatPaymentApp() : PaymentApp(0, kBatPaymentAppType) {
  app_method_names_.insert(kBatPaymentMethod);
}

BatPaymentApp::~BatPaymentApp() = default;

void BatPaymentApp::InvokePaymentApp(Delegate* delegate) {
  delegate->OnInstrumentDetailsReady(
      kBatPaymentMethod,
      details_,
      PayerData());
}

bool BatPaymentApp::IsCompleteForPayment() const {
  return true;
}

uint32_t BatPaymentApp::GetCompletenessScore() const {
  return 0;
}

bool BatPaymentApp::CanPreselect() const {
  return false;
}

base::string16 BatPaymentApp::GetMissingInfoLabel() const {
  return base::string16();
}

bool BatPaymentApp::HasEnrolledInstrument() const {
  return true;
}

void BatPaymentApp::RecordUse() {}

bool BatPaymentApp::NeedsInstallation() const {
  return false;
}

base::string16 BatPaymentApp::GetLabel() const {
  return base::string16();
}

base::string16 BatPaymentApp::GetSublabel() const {
  return base::string16();
}

bool BatPaymentApp::IsValidForModifier(
    const std::string& method,
    bool supported_networks_specified,
    const std::set<std::string>& supported_networks) const {
  bool is_valid = false;
  IsValidForPaymentMethodIdentifier(method, &is_valid);
  return is_valid;
}

base::WeakPtr<PaymentApp> BatPaymentApp::AsWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

bool BatPaymentApp::HandlesShippingAddress() const {
  return false;
}

bool BatPaymentApp::HandlesPayerName() const {
  return false;
}

bool BatPaymentApp::HandlesPayerEmail() const {
  return false;
}

bool BatPaymentApp::HandlesPayerPhone() const {
  return false;
}

void BatPaymentApp::SetResponseData(const std::string& order_id) {
  base::Value value(base::Value::Type::DICTIONARY);
  value.SetStringKey("orderId", order_id);
  base::JSONWriter::Write(value, &details_);
}

}  // namespace payments
