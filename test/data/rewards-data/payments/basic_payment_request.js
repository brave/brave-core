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

const request = new PaymentRequest(supportedInstruments, details)

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

function isBatMethodSupported() {
  run(() => {
    return request.canMakePayment();
  });
}
