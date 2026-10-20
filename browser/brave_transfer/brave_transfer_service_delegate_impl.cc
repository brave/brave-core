// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_transfer/brave_transfer_service_delegate_impl.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_transfer/browser/directory_file_sink.h"
#include "brave/components/brave_transfer/core/pref_names.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/download/download_prefs.h"
#include "chrome/browser/notifications/notification_display_service.h"
#include "chrome/browser/notifications/notification_display_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/singleton_tabs.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "components/services/quarantine/quarantine.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/image_model.h"
#include "ui/message_center/public/cpp/notification.h"
#include "ui/message_center/public/cpp/notification_delegate.h"
#include "url/gurl.h"

namespace brave_transfer {

namespace {

constexpr char kNotifierId[] = "service.brave_transfer";

DirectoryFileSink::Directories GetDirectories(base::WeakPtr<Profile> profile) {
  DirectoryFileSink::Directories directories;
  if (!profile) {
    return directories;
  }
  directories.preferred =
      profile->GetPrefs()->GetFilePath(prefs::kSaveDirectory);
  directories.fallback =
      DownloadPrefs::FromBrowserContext(profile.get())->DownloadPath();
  return directories;
}

// Marks a received file as coming from another device, like a download, so
// that Gatekeeper and SmartScreen check it before it is opened.
void QuarantineReceivedFile(const base::FilePath& path) {
  quarantine::QuarantineFile(path, GURL(), GURL(), std::nullopt, std::string(),
                             base::DoNothing());
}

void OpenTransferPage(base::WeakPtr<Profile> profile) {
  if (profile) {
    ShowSingletonTab(profile.get(), GURL(kBraveTransferURL));
  }
}

}  // namespace

std::unique_ptr<IncomingFileSink> CreateDownloadsFileSink(Profile* profile) {
  return std::make_unique<DirectoryFileSink>(
      base::BindRepeating(&GetDirectories, profile->GetWeakPtr()),
      base::BindRepeating(&QuarantineReceivedFile));
}

BraveTransferServiceDelegateImpl::BraveTransferServiceDelegateImpl(
    Profile* profile)
    : profile_(profile) {}

BraveTransferServiceDelegateImpl::~BraveTransferServiceDelegateImpl() = default;

base::FilePath BraveTransferServiceDelegateImpl::GetDefaultSaveDirectory() {
  return DownloadPrefs::FromBrowserContext(profile_)->DownloadPath();
}

void BraveTransferServiceDelegateImpl::NotifyOfferPending(
    const mojom::IncomingOffer& offer) {
  ShowNotification("brave_transfer.offer." + offer.id,
                   base::UTF8ToUTF16(offer.device_name),
                   l10n_util::GetPluralStringFUTF16(
                       IDS_BRAVE_TRANSFER_NOTIFICATION_OFFER,
                       base::saturated_cast<int>(offer.items.size())));
}

void BraveTransferServiceDelegateImpl::NotifyItemsReceived(
    const std::string& device_name,
    size_t count) {
  ShowNotification(
      "brave_transfer.received", base::UTF8ToUTF16(device_name),
      l10n_util::GetPluralStringFUTF16(IDS_BRAVE_TRANSFER_NOTIFICATION_RECEIVED,
                                       base::saturated_cast<int>(count)));
}

void BraveTransferServiceDelegateImpl::NotifyKeyMismatch(
    const std::string& device_name) {
  ShowNotification(
      "brave_transfer.key_mismatch", base::UTF8ToUTF16(device_name),
      l10n_util::GetStringUTF16(IDS_BRAVE_TRANSFER_NOTIFICATION_KEY_MISMATCH));
}

void BraveTransferServiceDelegateImpl::ShowNotification(
    const std::string& id,
    const std::u16string& title,
    const std::u16string& message) {
  auto* display_service =
      NotificationDisplayServiceFactory::GetForProfile(profile_);
  if (!display_service) {
    return;
  }
  message_center::RichNotificationData data;
  data.remove_on_click = true;
  // Keeps the notifier from showing an origin.
  data.context_message = u" ";
  message_center::Notification notification(
      message_center::NOTIFICATION_TYPE_SIMPLE, id, title, message,
      ui::ImageModel(), l10n_util::GetStringUTF16(IDS_BRAVE_TRANSFER_TITLE),
      GURL(kBraveTransferURL),
      message_center::NotifierId(message_center::NotifierType::SYSTEM_COMPONENT,
                                 kNotifierId),
      data,
      base::MakeRefCounted<message_center::HandleNotificationClickDelegate>(
          base::BindRepeating(&OpenTransferPage, profile_->GetWeakPtr())));
  display_service->Display(NotificationHandler::Type::TRANSIENT, notification,
                           /*metadata=*/nullptr);
}

}  // namespace brave_transfer
