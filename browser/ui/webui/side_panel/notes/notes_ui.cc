// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/side_panel/notes/notes_ui.h"

#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/browser/resources/notes/grit/notes_static_resources.h"
#include "brave/browser/resources/notes/grit/notes_static_resources_map.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "services/network/public/mojom/content_security_policy.mojom.h"
#include "ui/webui/webui_util.h"

NotesUI::NotesUI(content::WebUI* web_ui)
    : ui::MojoWebUIController(web_ui) {
  service_ = sidebar::SidebarServiceFactory::GetForProfile(
      Profile::FromWebUI(web_ui));
  service_->AddObserver(this);

  content::WebUIDataSource* source =
      content::WebUIDataSource::CreateAndAdd(
          Profile::FromWebUI(web_ui), "notes-panel");
  webui::SetupWebUIDataSource(
      source, {}, IDR_NOTES_STATIC_NOTES_HTML);
  source->AddResourcePaths(kNotesStaticResources);
}

NotesUI::~NotesUI() {
  if (service_) {
    service_->RemoveObserver(this);
  }
}

void NotesUI::BindInterface(
    mojo::PendingReceiver<notes::mojom::PageHandlerFactory> receiver) {
  factory_receiver_.reset();
  factory_receiver_.Bind(std::move(receiver));
}

void NotesUI::CreatePageHandler(
    mojo::PendingRemote<notes::mojom::Page> page,
    mojo::PendingReceiver<notes::mojom::PageHandler> handler) {
  page_.Bind(std::move(page));
  handler_receiver_.reset();
  handler_receiver_.Bind(std::move(handler));
}

std::vector<notes::mojom::NotePtr> NotesUI::GetMojoNotes() const {
  std::vector<notes::mojom::NotePtr> result;
  for (const sidebar::Notes& note : service_->GetNotes()) {
    auto mojo_note = notes::mojom::Note::New();
    mojo_note->id = note.id;
    mojo_note->title = note.title;
    mojo_note->content = note.content;
    mojo_note->timestamp = note.timestamp;
    result.push_back(std::move(mojo_note));
  }
  return result;
}

void NotesUI::GetNotes(GetNotesCallback callback) {
  std::move(callback).Run(GetMojoNotes());
}

void NotesUI::AddNote(notes::mojom::NotePtr note) {
  sidebar::Notes native_note;
  native_note.id = note->id;
  native_note.title = note->title;
  native_note.content = note->content;
  native_note.timestamp = note->timestamp;
  service_->AddNote(native_note);
}

void NotesUI::UpdateNote(notes::mojom::NotePtr note) {
  sidebar::Notes native_note;
  native_note.id = note->id;
  native_note.title = note->title;
  native_note.content = note->content;
  native_note.timestamp = note->timestamp;
  service_->UpdateNote(native_note);
}

void NotesUI::DeleteNote(const std::string& id) {
  service_->DeleteNote(id);
}

void NotesUI::NotifyNotesChanged() {
  if (page_.is_bound()) {
    page_->OnNotesChanged(GetMojoNotes());
  }
}

void NotesUI::OnNoteAdded(const sidebar::Notes&) {
  NotifyNotesChanged();
}

void NotesUI::OnNoteUpdated(const sidebar::Notes&) {
  NotifyNotesChanged();
}

void NotesUI::OnNoteDeleted(const std::string&) {
  NotifyNotesChanged();
}

WEB_UI_CONTROLLER_TYPE_IMPL(NotesUI)
