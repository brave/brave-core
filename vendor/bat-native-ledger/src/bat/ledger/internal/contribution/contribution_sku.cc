/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "bat/ledger/internal/common/bind_util.h"
#include "bat/ledger/internal/contribution/contribution_sku.h"
#include "bat/ledger/internal/contribution/contribution_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/static_values.h"
#include "bat/ledger/internal/uphold/uphold_util.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace {

const char kACSKUDev[] = "MDAxN2xvY2F0aW9uIGJyYXZlLmNvbQowMDFhaWRlbnRpZmllciBwdWJsaWMga2V5CjAwMzJjaWQgaWQgPSA1Yzg0NmRhMS04M2NkLTRlMTUtOThkZC04ZTE0N2E1NmI2ZmEKMDAxN2NpZCBjdXJyZW5jeSA9IEJBVAowMDE1Y2lkIHByaWNlID0gMC4yNQowMDJmc2lnbmF0dXJlICRlYyTuJdmlRFuPJ5XFQXjzHFZCLTek0yQ3Yc8JUKC0Cg";  //NOLINT
const char kACSKUStaging[] = "MDAxN2xvY2F0aW9uIGJyYXZlLmNvbQowMDFhaWRlbnRpZmllciBwdWJsaWMga2V5CjAwMzJjaWQgaWQgPSA1Yzg0NmRhMS04M2NkLTRlMTUtOThkZC04ZTE0N2E1NmI2ZmEKMDAxN2NpZCBjdXJyZW5jeSA9IEJBVAowMDE1Y2lkIHByaWNlID0gMC4yNQowMDJmc2lnbmF0dXJlICRlYyTuJdmlRFuPJ5XFQXjzHFZCLTek0yQ3Yc8JUKC0Cg";  //NOLINT
const char kACSKUProduction[] = "MDAxN2xvY2F0aW9uIGJyYXZlLmNvbQowMDFhaWRlbnRpZmllciBwdWJsaWMga2V5CjAwMzJjaWQgaWQgPSA1Yzg0NmRhMS04M2NkLTRlMTUtOThkZC04ZTE0N2E1NmI2ZmEKMDAxN2NpZCBjdXJyZW5jeSA9IEJBVAowMDE1Y2lkIHByaWNlID0gMC4yNQowMDJmc2lnbmF0dXJlICRlYyTuJdmlRFuPJ5XFQXjzHFZCLTek0yQ3Yc8JUKC0Cg";  //NOLINT

const char kUserFundsSKUDev[] = "MDAxN2xvY2F0aW9uIGJyYXZlLmNvbQowMDFhaWRlbnRpZmllciBwdWJsaWMga2V5CjAwMzJjaWQgaWQgPSA1Yzg0NmRhMS04M2NkLTRlMTUtOThkZC04ZTE0N2E1NmI2ZmEKMDAxN2NpZCBjdXJyZW5jeSA9IEJBVAowMDE1Y2lkIHByaWNlID0gMC4yNQowMDJmc2lnbmF0dXJlICRlYyTuJdmlRFuPJ5XFQXjzHFZCLTek0yQ3Yc8JUKC0Cg";  //NOLINT
const char kUserFundsSKUStaging[] = "MDAxN2xvY2F0aW9uIGJyYXZlLmNvbQowMDFhaWRlbnRpZmllciBwdWJsaWMga2V5CjAwMzJjaWQgaWQgPSA1Yzg0NmRhMS04M2NkLTRlMTUtOThkZC04ZTE0N2E1NmI2ZmEKMDAxN2NpZCBjdXJyZW5jeSA9IEJBVAowMDE1Y2lkIHByaWNlID0gMC4yNQowMDJmc2lnbmF0dXJlICRlYyTuJdmlRFuPJ5XFQXjzHFZCLTek0yQ3Yc8JUKC0Cg";  //NOLINT
const char kUserFundsSKUProduction[] = "MDAxN2xvY2F0aW9uIGJyYXZlLmNvbQowMDFhaWRlbnRpZmllciBwdWJsaWMga2V5CjAwMzJjaWQgaWQgPSA1Yzg0NmRhMS04M2NkLTRlMTUtOThkZC04ZTE0N2E1NmI2ZmEKMDAxN2NpZCBjdXJyZW5jeSA9IEJBVAowMDE1Y2lkIHByaWNlID0gMC4yNQowMDJmc2lnbmF0dXJlICRlYyTuJdmlRFuPJ5XFQXjzHFZCLTek0yQ3Yc8JUKC0Cg";  //NOLINT

const char kAnonCardDestinationDev[] = "2c326b15-7106-48be-a326-06f19e69746b";  //NOLINT
const char kAnonCardDestinationStaging[] = "2c326b15-7106-48be-a326-06f19e69746b";  //NOLINT
const char kAnonCardDestinationProduction[] = "2c326b15-7106-48be-a326-06f19e69746b";  //NOLINT

std::string GetACSKU() {
  if (ledger::_environment == ledger::Environment::PRODUCTION) {
    return kACSKUProduction;
  }

  if (ledger::_environment == ledger::Environment::STAGING) {
    return kACSKUStaging;
  }

  if (ledger::_environment == ledger::Environment::DEVELOPMENT) {
    return kACSKUDev;
  }

  NOTREACHED();
  return kACSKUDev;
}

std::string GetUserFundsSKU() {
  if (ledger::_environment == ledger::Environment::PRODUCTION) {
    return kUserFundsSKUProduction;
  }

  if (ledger::_environment == ledger::Environment::STAGING) {
    return kUserFundsSKUStaging;
  }

  if (ledger::_environment == ledger::Environment::DEVELOPMENT) {
    return kUserFundsSKUDev;
  }

  NOTREACHED();
  return kUserFundsSKUDev;
}

std::string GetAnonCardDestination() {
  if (ledger::_environment == ledger::Environment::PRODUCTION) {
    return kAnonCardDestinationProduction;
  }

  if (ledger::_environment == ledger::Environment::STAGING) {
    return kAnonCardDestinationStaging;
  }

  if (ledger::_environment == ledger::Environment::DEVELOPMENT) {
    return kAnonCardDestinationDev;
  }

  NOTREACHED();
  return kAnonCardDestinationDev;
}

}  // namespace

namespace braveledger_contribution {

ContributionSKU::ContributionSKU(
    bat_ledger::LedgerImpl* ledger,
    Contribution* contribution):
    ledger_(ledger),
    contribution_(contribution) {
  DCHECK(ledger_);
  credentials_ = braveledger_credentials::CredentialsFactory::Create(
      ledger_,
      ledger::CredsBatchType::SKU);
  DCHECK(credentials_);
}

ContributionSKU::~ContributionSKU() = default;

void ContributionSKU::AutoContribution(
    const std::string& contribution_id,
    ledger::ExternalWalletPtr wallet) {
  ledger::SKUOrderItem item;
  item.sku = GetACSKU();
  item.price = braveledger_ledger::_vote_price;

  Start(
      contribution_id,
      item,
      braveledger_uphold::GetACAddress(),
      std::move(wallet));
}

void ContributionSKU::AnonUserFunds(
    const std::string& contribution_id,
    ledger::ExternalWalletPtr wallet) {
  ledger::SKUOrderItem item;
  item.sku = GetUserFundsSKU();
  item.price = braveledger_ledger::_vote_price;

  Start(
      contribution_id,
      item,
      GetAnonCardDestination(),
      std::move(wallet));
}

void ContributionSKU::Start(
    const std::string& contribution_id,
    const ledger::SKUOrderItem& item,
    const std::string& destination,
    ledger::ExternalWalletPtr wallet) {
  auto get_callback = std::bind(&ContributionSKU::GetContributionInfo,
      this,
      _1,
      item,
      destination,
      *wallet);

  ledger_->GetContributionInfo(contribution_id, get_callback);
}

void ContributionSKU::GetContributionInfo(
    ledger::ContributionInfoPtr contribution,
    const ledger::SKUOrderItem& item,
    const std::string& destination,
    const ledger::ExternalWallet& wallet) {
  if (!contribution) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Contribution not found";
    return;
  }

  ledger::ResultCallback complete_callback = std::bind(
      &ContributionSKU::Completed,
      this,
      _1,
      contribution->contribution_id,
      contribution->type);

  auto process_callback = std::bind(&ContributionSKU::GetOrder,
      this,
      _1,
      _2,
      complete_callback);

  ledger::SKUOrderItem new_item = item;
  new_item.quantity = GetVotesFromAmount(contribution->amount);
  new_item.type = ledger::SKUOrderItemType::SINGLE_USE;

  std::vector<ledger::SKUOrderItem> items;
  items.push_back(new_item);

  ledger_->BraveSKU(
      destination,
      items,
      contribution->contribution_id,
      ledger::ExternalWallet::New(wallet),
      process_callback);
}

void ContributionSKU::GetOrder(
    const ledger::Result result,
    const std::string& order_id,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "SKU was not processed";
    callback(result);
    return;
  }

  auto get_callback = std::bind(&ContributionSKU::OnGetOrder,
      this,
      _1,
      callback);
  ledger_->GetSKUOrder(order_id, get_callback);
}

void ContributionSKU::OnGetOrder(
    ledger::SKUOrderPtr order,
    ledger::ResultCallback callback) {
  if (!order || order->items.empty()) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Order was not found";
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  DCHECK_EQ(order->items.size(), 1ul);

  std::vector<std::string> data;
  data.push_back(order->items[0]->order_item_id);
  data.push_back(std::to_string(static_cast<int>(order->items[0]->type)));

  braveledger_credentials::CredentialsTrigger trigger;
  trigger.id = order->order_id;
  trigger.size = order->items[0]->quantity;
  trigger.type = ledger::CredsBatchType::SKU;
  trigger.data = data;

  credentials_->Start(trigger, callback);
}

void ContributionSKU::Completed(
    const ledger::Result result,
    const std::string& contribution_id,
    const ledger::RewardsType type) {
  if (result != ledger::Result::LEDGER_OK) {
    ledger_->ContributionCompleted(result, 0, contribution_id, type);
    return;
  }

  contribution_->StartUnblinded(contribution_id);
}

}  // namespace braveledger_contribution
