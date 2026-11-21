// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_SIDE_PANEL_NOTES_NOTES_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_SIDE_PANEL_NOTES_NOTES_UI_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "brave/browser/ui/webui/side_panel/notes/notes.mojom.h"
#include "brave/components/sidebar/browser/sidebar_service.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "ui/webui/mojo_web_ui_controller.h"

class NotesUI : public ui::MojoWebUIController,
                public notes::mojom::PageHandlerFactory,
                public notes::mojom::PageHandler,
                public sidebar::SidebarService::Observer {
 public:
  explicit NotesUI(content::WebUI* web_ui);
  ~NotesUI() override;

  void BindInterface(
      mojo::PendingReceiver<notes::mojom::PageHandlerFactory> receiver);

  // notes::mojom::PageHandlerFactory:
  void CreatePageHandler(
      mojo::PendingRemote<notes::mojom::Page> page,
      mojo::PendingReceiver<notes::mojom::PageHandler> handler) override;

  // notes::mojom::PageHandler:
  void GetNotes(GetNotesCallback callback) override;
  void AddNote(notes::mojom::NotePtr note) override;
  void UpdateNote(notes::mojom::NotePtr note) override;
  void DeleteNote(const std::string& id) override;

  // sidebar::SidebarService::Observer:
  void OnNoteAdded(const sidebar::Notes& note) override;
  void OnNoteUpdated(const sidebar::Notes& note) override;
  void OnNoteDeleted(const std::string& id) override;

 private:
  void NotifyNotesChanged();
  std::vector<notes::mojom::NotePtr> GetMojoNotes() const;

  raw_ptr<sidebar::SidebarService> service_ = nullptr;
  mojo::Receiver<notes::mojom::PageHandlerFactory>
      factory_receiver_{this};
  mojo::Receiver<notes::mojom::PageHandler> handler_receiver_{this};
  mojo::Remote<notes::mojom::Page> page_;

  WEB_UI_CONTROLLER_TYPE_DECL();
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SIDE_PANEL_NOTES_NOTES_UI_H_
