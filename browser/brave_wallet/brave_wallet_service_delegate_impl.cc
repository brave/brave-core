/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_service_delegate_impl.h"

#include <optional>
#include <utility>

#include "base/check.h"
#include "base/functional/callback_helpers.h"
#include "base/strings/utf_string_conversions.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/permission_utils.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/permissions/contexts/brave_wallet_permission_context.h"
#include "chrome/browser/notifications/notification_display_service.h"
#include "chrome/browser/notifications/notification_display_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface_iterator.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/message_center/public/cpp/notification.h"
#include "url/gurl.h"
#include "url/origin.h"

using content::StoragePartition;

namespace brave_wallet {

namespace {

content::WebContents* g_web_contents_for_testing = nullptr;

content::WebContents* GetActiveWebContents() {
  if (g_web_contents_for_testing) {
    return g_web_contents_for_testing;
  }

  BrowserWindowInterface* browser =
      GetLastActiveBrowserWindowInterfaceWithAnyProfile();
  return browser ? browser->GetTabStripModel()->GetActiveWebContents()
                 : nullptr;
}

void ClearWalletStoragePartition(content::BrowserContext* context,
                                 const GURL& url) {
  CHECK(context);
  auto* partition = context->GetDefaultStoragePartition();
  partition->ClearDataForOrigin(StoragePartition::REMOVE_DATA_MASK_ALL, url,
                                base::DoNothing());
}

std::u16string GetStatusTitle(brave_wallet::mojom::TransactionStatus status) {
  switch (status) {
    case brave_wallet::mojom::TransactionStatus::Confirmed:
      return l10n_util::GetStringUTF16(
          IDS_WALLET_TRANSACTION_STATUS_UPDATE_MESSAGE_TITLE_CONFIRMED);
    case brave_wallet::mojom::TransactionStatus::Error:
      return l10n_util::GetStringUTF16(
          IDS_WALLET_TRANSACTION_STATUS_UPDATE_MESSAGE_TITLE_ERROR);
    case brave_wallet::mojom::TransactionStatus::Dropped:
      return l10n_util::GetStringUTF16(
          IDS_WALLET_TRANSACTION_STATUS_UPDATE_MESSAGE_TITLE_DROPPED);
    default:
      break;
  }

  return std::u16string();
}

void DisplayTxNotificationImpl(content::BrowserContext* context,
                               brave_wallet::mojom::TransactionStatus status,
                               const std::string& account_name,
                               const std::string& tx_id,
                               const GURL& tx_url) {
  if (auto* notification_display_service =
          NotificationDisplayServiceFactory::GetForProfile(
              Profile::FromBrowserContext(context))) {
    message_center::RichNotificationData notification_data;
    notification_data.remove_on_click = true;
    notification_data.context_message =
        u" ";  // Prevent origin from showing in the notification.

    auto notification = std::make_unique<message_center::Notification>(
        message_center::NOTIFICATION_TYPE_SIMPLE, tx_id, GetStatusTitle(status),
        l10n_util::GetStringFUTF16(
            IDS_WALLET_TRANSACTION_STATUS_UPDATE_MESSAGE_TEXT,
            base::UTF8ToUTF16(account_name)),
        ui::ImageModel(), std::u16string(), tx_url,
        message_center::NotifierId(
            message_center::NotifierType::SYSTEM_COMPONENT, "service.wallet"),
        notification_data, nullptr);

    notification_display_service->Display(
        NotificationHandler::Type::BRAVE_WALLET, *notification, nullptr);
  }
}

}  // namespace

BraveWalletServiceDelegateImpl::BraveWalletServiceDelegateImpl(
    content::BrowserContext* context)
    : BraveWalletServiceDelegateBase(context),
      browser_tab_strip_tracker_(this, this),
      weak_ptr_factory_(this) {
  browser_tab_strip_tracker_.Init();
}

BraveWalletServiceDelegateImpl::~BraveWalletServiceDelegateImpl() = default;

// static
void BraveWalletServiceDelegateImpl::SetActiveWebContentsForTesting(
    content::WebContents* web_contents) {
  g_web_contents_for_testing = web_contents;
}

void BraveWalletServiceDelegateImpl::AddObserver(
    BraveWalletServiceDelegate::Observer* observer) {
  observer_list_.AddObserver(observer);
}

void BraveWalletServiceDelegateImpl::RemoveObserver(
    BraveWalletServiceDelegate::Observer* observer) {
  observer_list_.RemoveObserver(observer);
}

bool BraveWalletServiceDelegateImpl::ShouldTrackBrowser(
    BrowserWindowInterface* browser) {
  return browser->GetProfile() == Profile::FromBrowserContext(context_);
}

void BraveWalletServiceDelegateImpl::IsExternalWalletInstalled(
    mojom::ExternalWalletType type,
    IsExternalWalletInstalledCallback callback) {
  ExternalWalletsImporter importer(type, context_);
  std::move(callback).Run(importer.IsExternalWalletInstalled());
}

void BraveWalletServiceDelegateImpl::IsExternalWalletInitialized(
    mojom::ExternalWalletType type,
    IsExternalWalletInitializedCallback callback) {
  importers_[type] = std::make_unique<ExternalWalletsImporter>(type, context_);
  // Do not try to init the importer when external wallet is not installed
  if (!importers_[type]->IsExternalWalletInstalled()) {
    std::move(callback).Run(false);
    return;
  }
  if (importers_[type]->IsInitialized()) {
    ContinueIsExternalWalletInitialized(type, std::move(callback), true);
  } else {
    importers_[type]->Initialize(base::BindOnce(
        &BraveWalletServiceDelegateImpl::ContinueIsExternalWalletInitialized,
        weak_ptr_factory_.GetWeakPtr(), type, std::move(callback)));
  }
}

void BraveWalletServiceDelegateImpl::ContinueIsExternalWalletInitialized(
    mojom::ExternalWalletType type,
    IsExternalWalletInitializedCallback callback,
    bool init_success) {
  DCHECK(importers_[type]);
  if (init_success) {
    std::move(callback).Run(importers_[type]->IsExternalWalletInitialized());
  } else {
    std::move(callback).Run(false);
  }
}

void BraveWalletServiceDelegateImpl::GetImportInfoFromExternalWallet(
    mojom::ExternalWalletType type,
    const std::string& password,
    GetImportInfoCallback callback) {
  if (!importers_[type]) {
    importers_[type] =
        std::make_unique<ExternalWalletsImporter>(type, context_);
  }
  if (importers_[type]->IsInitialized()) {
    ContinueGetImportInfoFromExternalWallet(type, password, std::move(callback),
                                            true);
  } else {
    importers_[type]->Initialize(base::BindOnce(
        &BraveWalletServiceDelegateImpl::
            ContinueGetImportInfoFromExternalWallet,
        weak_ptr_factory_.GetWeakPtr(), type, password, std::move(callback)));
  }
}

void BraveWalletServiceDelegateImpl::ContinueGetImportInfoFromExternalWallet(
    mojom::ExternalWalletType type,
    const std::string& password,
    GetImportInfoCallback callback,
    bool init_success) {
  DCHECK(importers_[type]);
  if (init_success) {
    DCHECK(importers_[type]->IsInitialized());
    importers_[type]->GetImportInfo(password, std::move(callback));
  } else {
    std::move(callback).Run(base::unexpected(ImportError::kInternalError));
  }
}

void BraveWalletServiceDelegateImpl::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  FireActiveOriginChanged();
}

void BraveWalletServiceDelegateImpl::OnTabChangedAt(tabs::TabInterface* tab,
                                                    int index,
                                                    TabChangeType change_type) {
  if (!tab || tab->GetContents() != GetActiveWebContents()) {
    return;
  }

  FireActiveOriginChanged();
}

void BraveWalletServiceDelegateImpl::FireActiveOriginChanged() {
  mojom::OriginInfoPtr origin_info =
      MakeOriginInfo(GetActiveOriginInternal().value_or(url::Origin()));
  for (auto& observer : observer_list_) {
    observer.OnActiveOriginChanged(origin_info);
  }
}

std::optional<url::Origin>
BraveWalletServiceDelegateImpl::GetActiveOriginInternal() {
  content::WebContents* contents = GetActiveWebContents();
  return contents ? contents->GetPrimaryMainFrame()->GetLastCommittedOrigin()
                  : std::optional<url::Origin>();
}

std::optional<url::Origin> BraveWalletServiceDelegateImpl::GetActiveOrigin() {
  return GetActiveOriginInternal();
}

void BraveWalletServiceDelegateImpl::ClearWalletUIStoragePartition() {
  ClearWalletStoragePartition(context_, GURL(kBraveUIWalletURL));
  ClearWalletStoragePartition(context_, GURL(kBraveUIWalletPanelURL));
}

void BraveWalletServiceDelegateImpl::DisplayTxNotification(
    brave_wallet::mojom::TransactionStatus status,
    const std::string& account_name,
    const std::string& tx_id,
    const GURL& tx_url) {
  DisplayTxNotificationImpl(context_, status, account_name, tx_id, tx_url);
}

}  // namespace brave_wallet
