// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_AGENT_TRACING_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_AGENT_TRACING_H_

#include <memory>
#include <string>
#include <string_view>

namespace ai_chat {

// Phase names, shared between the trace spans and the journal entries so that a
// Perfetto trace and chrome://actor-internals can be read side by side.
inline constexpr char kAgentPhaseToolLoop[] = "AIChat.ToolLoop";
inline constexpr char kAgentPhaseGeneration[] = "AIChat.Generation";
inline constexpr char kAgentPhaseToolUse[] = "AIChat.ToolUse";
inline constexpr char kAgentPhaseAssociatedContent[] =
    "AIChat.AssociatedContentExtraction";
inline constexpr char kAgentPhaseTabSetup[] = "AIChat.TabSetup";
inline constexpr char kAgentPhaseActuation[] = "AIChat.Actuation";
inline constexpr char kAgentPhaseObservation[] = "AIChat.Observation";
inline constexpr char kAgentPhaseObservationSerialization[] =
    "AIChat.ObservationSerialization";

// Instant event names.
inline constexpr char kAgentEventActionResult[] = "AIChat.ActionResult";

// Sink for the parts of agentic loop timing that only the browser layer can
// reach: the actor journal behind chrome://actor-internals. Entries are scoped
// to an actor task, and components/ai_chat can depend on neither
// chrome/browser/actor nor components/actor, so the loop records through this
// interface and brave/browser/ai_chat implements it.
class AgentJournal {
 public:
  // An in-flight phase. Destroying it ends the journal entry.
  class PendingEntry {
   public:
    virtual ~PendingEntry() = default;

    // Ends the entry with `details`. Only called once; destroying a
    // PendingEntry that was never ended ends it without details.
    virtual void End(std::string_view details) = 0;
  };

  virtual ~AgentJournal() = default;

  virtual std::unique_ptr<PendingEntry> Begin(std::string_view event_name,
                                              std::string_view details) = 0;
  virtual void Log(std::string_view event_name, std::string_view details) = 0;
};

// Times one phase of the agentic loop: always a `brave.ai_chat` trace span,
// plus a journal entry when `journal` is non-null. The phase ends when this
// object is destroyed, so hold it for as long as the phase runs - loop phases
// are asynchronous and routinely begin and end in different methods.
class AgentPhase {
 public:
  // `name` must outlive this object, so use the constants above. `journal` may
  // be null and is only used during construction.
  AgentPhase(AgentJournal* journal, const char* name, std::string_view details);
  AgentPhase(const AgentPhase&) = delete;
  AgentPhase& operator=(const AgentPhase&) = delete;
  ~AgentPhase();

  // Recorded against the end of the phase, e.g. its outcome or result size.
  void SetEndDetails(std::string details);

 private:
  const char* name_;
  std::string end_details_;
  std::unique_ptr<AgentJournal::PendingEntry> entry_;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_AGENT_TRACING_H_
