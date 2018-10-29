/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/importer/brave_import_lock_dialog_view.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/metrics/user_metrics.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "brave/grit/brave_generated_resources.h"
#include "build/build_config.h"
#include "chrome/browser/importer/importer_lock_dialog.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/locale_settings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/ui_features.h"
#include "ui/views/border.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/widget/widget.h"

using base::UserMetricsAction;

namespace brave {
namespace importer {

void ShowImportLockDialog(gfx::NativeWindow parent,
                          ::importer::SourceProfile source_profile,
                          const base::Callback<void(bool)>& callback) {
  ImportLockDialogView::Show(parent, source_profile, callback);
}

}  // namespace importer

// static
void ImportLockDialogView::Show(gfx::NativeWindow parent,
                                ::importer::SourceProfile source_profile,
                                const base::Callback<void(bool)>& callback) {
  views::DialogDelegate::CreateDialogWidget(
      new ImportLockDialogView(source_profile, callback), NULL, NULL)->Show();
  base::RecordAction(UserMetricsAction("ImportLockDialogView_Shown"));
}

ImportLockDialogView::ImportLockDialogView(
    ::importer::SourceProfile source_profile,
    const base::Callback<void(bool)>& callback)
  : source_profile_(source_profile),
    callback_(callback) {
  SetLayoutManager(std::make_unique<views::FillLayout>());

  views::Label* description_label;
  if (source_profile_.importer_type == ::importer::TYPE_CHROME) {
    description_label =
      new views::Label(l10n_util::GetStringUTF16(IDS_CHROME_IMPORTER_LOCK_TEXT));
  } else {  // if (source_profile_.importer_type == ::importer::TYPE_BRAVE)
    description_label =
      new views::Label(l10n_util::GetStringUTF16(IDS_BRAVE_IMPORTER_LOCK_TEXT));
  }
  description_label->SetBorder(views::CreateEmptyBorder(
      ChromeLayoutProvider::Get()->GetDialogInsetsForContentType(views::TEXT,
                                                                 views::TEXT)));
  description_label->SetMultiLine(true);
  description_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  AddChildView(description_label);
  chrome::RecordDialogCreation(chrome::DialogIdentifier::IMPORT_LOCK);
}

ImportLockDialogView::~ImportLockDialogView() {
}

gfx::Size ImportLockDialogView::CalculatePreferredSize() const {
  const int width = ChromeLayoutProvider::Get()->GetDistanceMetric(
      DISTANCE_MODAL_DIALOG_PREFERRED_WIDTH);
  return gfx::Size(width, GetHeightForWidth(width));
}

base::string16 ImportLockDialogView::GetDialogButtonLabel(
    ui::DialogButton button) const {
  if (button == ui::DIALOG_BUTTON_OK)
    return l10n_util::GetStringUTF16(IDS_IMPORTER_LOCK_OK);
  return DialogDelegateView::GetDialogButtonLabel(button);
}

base::string16 ImportLockDialogView::GetWindowTitle() const {
  if (source_profile_.importer_type == ::importer::TYPE_CHROME) {
    return l10n_util::GetStringUTF16(IDS_CHROME_IMPORTER_LOCK_TITLE);
  } else {  // if (source_profile_.importer_type == ::importer::TYPE_BRAVE) {
    return l10n_util::GetStringUTF16(IDS_BRAVE_IMPORTER_LOCK_TITLE);
  }
}

bool ImportLockDialogView::Accept() {
  if (callback_) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(callback_, true));
  }
  return true;
}

bool ImportLockDialogView::Cancel() {
  if (callback_) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(callback_, false));
  }
  return true;
}

bool ImportLockDialogView::ShouldShowCloseButton() const {
  return false;
}

}  // namespace brave
