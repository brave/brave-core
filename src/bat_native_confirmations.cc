/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat_native_confirmations.h"
#include "mock_confirmations_client.h"
#include "bat/confirmations/confirmations.h"

std::string happy_data; 
int happy_status;
int count;

int main() {
  auto mock_confirmations_client =
    std::make_unique<confirmations::MockConfirmationsClient>();
  confirmations::Confirmations& confirmations =
    *mock_confirmations_client->confirmations_;

  confirmations.Initialize();



  return 0;
}
