// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/agent_tracing.h"

#include <utility>

#include "base/trace_event/trace_event.h"

namespace ai_chat {

namespace {

// A track per live phase, so overlapping phases (a generation inside a tool
// loop) each get their own row rather than being flattened into one.
perfetto::NamedTrack PhaseTrack(const char* name, const void* phase) {
  return perfetto::NamedTrack::FromPointer(perfetto::StaticString{name}, phase);
}

}  // namespace

AgentPhase::AgentPhase(AgentJournal* journal,
                       const char* name,
                       std::string_view details)
    : name_(name) {
  TRACE_EVENT_BEGIN("brave.ai_chat", perfetto::StaticString{name_},
                    PhaseTrack(name_, this), "details", std::string(details));
  if (journal) {
    entry_ = journal->Begin(name_, details);
  }
}

AgentPhase::~AgentPhase() {
  if (entry_) {
    entry_->End(end_details_);
  }
  TRACE_EVENT_END("brave.ai_chat", PhaseTrack(name_, this), "details",
                  end_details_);
}

void AgentPhase::SetEndDetails(std::string details) {
  end_details_ = std::move(details);
}

}  // namespace ai_chat
