/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Cocoa/Cocoa.h>

#include "base/bind.h"
#include "base/callback.h"
#include "base/mac/scoped_nsobject.h"
#include "base/metrics/user_metrics.h"
#include "base/threading/thread_task_runner_handle.h"
#include "brave/browser/importer/brave_importer_lock_dialog.h"
#include "brave/browser/ui/views/importer/brave_import_lock_dialog_view.h"
#include "chrome/browser/ui/cocoa/browser_dialogs_views_mac.h"
#include "chrome/common/importer/importer_data_types.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util_mac.h"

using base::UserMetricsAction;

namespace brave {
namespace importer {

void ShowImportLockDialog(gfx::NativeWindow parent,
                          importer::SourceProfile source_profile,
                          const base::Callback<void(bool)>& callback) {
  if (chrome::ShowAllDialogsWithViewsToolkit())
    return ImportLockDialogView::Show(parent, source_profile, callback);

  base::scoped_nsobject<NSAlert> lock_alert([[NSAlert alloc] init]);
  [lock_alert addButtonWithTitle:l10n_util::GetNSStringWithFixup(
      IDS_IMPORTER_LOCK_OK)];
  [lock_alert addButtonWithTitle:l10n_util::GetNSStringWithFixup(IDS_CANCEL)];

  if (source_profile.importer_type == importer::TYPE_CHROME) {
    [lock_alert setInformativeText:l10n_util::GetNSStringWithFixup(
        IDS_CHROME_IMPORTER_LOCK_TEXT)];
    [lock_alert setMessageText:l10n_util::GetNSStringWithFixup(
        IDS_CHROME_IMPORTER_LOCK_TITLE)];
  } else if (source_profile.importer_type == importer::TYPE_BRAVE) {
    [lock_alert setInformativeText:l10n_util::GetNSStringWithFixup(
        IDS_BRAVE_IMPORTER_LOCK_TEXT)];
    [lock_alert setMessageText:l10n_util::GetNSStringWithFixup(
        IDS_BRAVE_IMPORTER_LOCK_TITLE)];
  }

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::Bind(callback, [lock_alert runModal] == NSAlertFirstButtonReturn));
  base::RecordAction(UserMetricsAction("ImportLockDialogCocoa_Shown"));
}

}  // namespace importer
}  // namespace brave
