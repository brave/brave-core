// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_SKUS_INTERNALS_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_SKUS_INTERNALS_UI_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/values.h"
#include "brave/components/skus/common/skus_internals.mojom.h"
#include "content/public/browser/web_ui_controller.h"
#include "ui/shell_dialogs/select_file_dialog.h"

class PrefService;

class SkusInternalsUI : public content::WebUIController,
                        public skus::mojom::SkusInternals,
                        public ui::SelectFileDialog::Listener {
 public:
  SkusInternalsUI(content::WebUI* web_ui, const std::string& host);
  ~SkusInternalsUI() override;
  SkusInternalsUI(const SkusInternalsUI&) = delete;
  SkusInternalsUI& operator=(const SkusInternalsUI&) = delete;

  void BindInterface(
      mojo::PendingReceiver<skus::mojom::SkusInternals> pending_receiver);

 private:
  // skus::mojom::SkusInternals overrides:
  void GetEventLog(GetEventLogCallback callback) override;
  void GetSkusState(GetSkusStateCallback callback) override;
  void GetVpnState(GetVpnStateCallback callback) override;
  void ResetSkusState() override;
  void CopySkusStateToClipboard() override;
  void DownloadSkusState() override;

  // SelectFileDialog::Listener overrides:
  void FileSelected(const ui::SelectedFileInfo& file,
                    int index,
                    void* params) override;
  void FileSelectionCanceled(void* params) override;

  std::string GetLastVPNConnectionError() const;
  base::Value::Dict GetVPNOrderInfo() const;
  std::string GetSkusStateAsString() const;

  raw_ptr<PrefService> local_state_ = nullptr;
  scoped_refptr<ui::SelectFileDialog> select_file_dialog_ = nullptr;
  mojo::Receiver<skus::mojom::SkusInternals> skus_internals_receiver_{this};

  WEB_UI_CONTROLLER_TYPE_DECL();
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SKUS_INTERNALS_UI_H_
