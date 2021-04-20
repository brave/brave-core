/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

const supportedInstruments = [{
    supportedMethods: 'bat',
}];

const details = {
  id: '305DEF20-3F33-4D10-B901-8AFF34EBA750',
  total: {label: 'Total', amount: {currency: 'BAT', value: '0.25'}},
  displayItems: [
    {
      label: 'Brave T-Shirt',
      amount: {currency: 'BAT', value: '0.25'},
      sku: 'test-sku'
    },
  ],
};

const details_without_display_items = {
  id: '305DEF20-3F33-4D10-B901-8AFF34EBA750',
  total: {label: 'Total', amount: {currency: 'BAT', value: '0.25'}},
};

const details_without_sku_tokens = {
  id: '305DEF20-3F33-4D10-B901-8AFF34EBA750',
  total: {label: 'Total', amount: {currency: 'BAT', value: '0.25'}},
  displayItems: [
    {
      label: 'Brave T-Shirt',
      amount: {currency: 'BAT', value: '0.25'}
    },
  ],
};

var basic_request;
var request_without_display_items;
var request_without_sku_tokens;

function load() {
  basic_request = new PaymentRequest(supportedInstruments, details);
  request_without_display_items = new PaymentRequest(supportedInstruments, details_without_display_items);
  request_without_sku_tokens = new PaymentRequest(supportedInstruments, details_without_sku_tokens);
}

function print(msg) {
  document.getElementById('result').innerHTML = msg;
}

function run(testFunction) {
  try {
    testFunction().then(print).catch(print);
  } catch (error) {
    print("false");
  }
}

function batPaymentMethodSupported() {
  run(() => {
    return basic_request.canMakePayment();
  });
}

function paymentRequestWithoutDisplayItems() {
  run(() => {
    return request_without_display_items.canMakePayment();
  });
}

function paymentRequestWithoutSkuTokens() {
  run(() => {
    return request_without_sku_tokens.canMakePayment();
  });
}

