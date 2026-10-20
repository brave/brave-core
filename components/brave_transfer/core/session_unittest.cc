// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_transfer/core/session.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/containers/to_vector.h"
#include "base/functional/bind.h"
#include "base/memory/raw_ref.h"
#include "base/types/expected_macros.h"
#include "brave/components/brave_transfer/core/frame_codec.h"
#include "brave/components/brave_transfer/core/test/test_vectors.h"
#include "crypto/hash.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_transfer {

namespace {

base::TimeTicks At(base::TimeDelta offset) {
  return base::TimeTicks() + offset;
}

const base::TimeTicks kZero = base::TimeTicks();

proto::Frame RoundTrip(const proto::Frame& frame) {
  auto decoded = DecodeFrame(EncodeMessage(frame));
  CHECK(decoded.has_value());
  return *decoded;
}

std::vector<proto::Frame> Drain(TransferSession& session,
                                base::TimeTicks now = kZero) {
  std::vector<proto::Frame> frames;
  while (true) {
    auto frame = session.NextFrame(now);
    CHECK(frame.has_value()) << frame.error();
    if (!frame->has_value()) {
      return frames;
    }
    frames.push_back(std::move(**frame));
  }
}

Result<TransferEvents> Deliver(const std::vector<proto::Frame>& frames,
                               TransferSession& session,
                               base::TimeTicks now = kZero) {
  TransferEvents all;
  for (const proto::Frame& frame : frames) {
    ASSIGN_OR_RETURN(TransferEvents events,
                     session.Handle(RoundTrip(frame), now));
    for (TransferEvent& event : events) {
      all.push_back(std::move(event));
    }
  }
  return all;
}

TransferEvents MustDeliver(const std::vector<proto::Frame>& frames,
                           TransferSession& session,
                           base::TimeTicks now = kZero) {
  auto events = Deliver(frames, session, now);
  CHECK(events.has_value()) << events.error();
  return std::move(*events);
}

std::vector<TransferResult> Finished(const TransferEvents& events) {
  std::vector<TransferResult> results;
  for (const TransferEvent& event : events) {
    if (const auto* finished = std::get_if<TransferFinished>(&event)) {
      results.push_back(finished->result);
    }
  }
  return results;
}

std::vector<OutgoingItem> VectorItems() {
  return test::TranscriptItems(test::LoadVectors("session_transcript.json"));
}

TransferSession::RandomCallback FixedRandom(Bytes bytes) {
  return base::BindRepeating(
      [](const Bytes& bytes, size_t n) {
        CHECK_LE(n, bytes.size());
        return Bytes(bytes.begin(), bytes.begin() + static_cast<ptrdiff_t>(n));
      },
      std::move(bytes));
}

std::string Offer(TransferSession& sender,
                  TransferSession& receiver,
                  std::vector<OutgoingItem> items = VectorItems()) {
  auto id = sender.SendOffer(std::move(items), kZero);
  CHECK(id.has_value()) << id.error();
  MustDeliver(Drain(sender), receiver);
  return *id;
}

proto::Frame ResponseFrame(const std::string& transfer_id,
                           proto::Decision decision,
                           const std::vector<uint32_t>& ids = {}) {
  proto::Frame frame;
  proto::Response* response = frame.mutable_response();
  response->set_transfer_id(transfer_id);
  response->set_decision(decision);
  for (uint32_t id : ids) {
    response->add_accepted_item_ids(id);
  }
  return frame;
}

proto::Frame CancelFrame(const std::string& transfer_id,
                         proto::CancelReason reason) {
  proto::Frame frame;
  frame.mutable_cancel()->set_transfer_id(transfer_id);
  frame.mutable_cancel()->set_reason(reason);
  return frame;
}

proto::Frame PingFrame(uint64_t sequence) {
  proto::Frame frame;
  frame.mutable_ping()->set_sequence(sequence);
  return frame;
}

std::string Encoded(const proto::Frame& frame) {
  return test::HexString(EncodeMessage(frame));
}

struct ScenarioRun {
  std::vector<std::pair<std::string, proto::Frame>> frames;
  std::map<std::string, TransferResult> results;
};

// Same delivery order as run_scenario() in tools/brave_transfer/gen_vectors.py.
ScenarioRun RunScenario(std::vector<OutgoingItem> items,
                        const Bytes& transfer_id,
                        proto::Decision decision,
                        const std::vector<uint32_t>& accepted_ids) {
  const base::TimeTicks now = At(base::Seconds(1000));
  TransferSession initiator(now, ReceiveLimits(), /*send_confirmation=*/false,
                            FixedRandom(transfer_id));
  TransferSession responder(now, ReceiveLimits(), /*send_confirmation=*/true);
  CHECK(initiator.SendOffer(std::move(items), now).has_value());
  ScenarioRun run;

  auto deliver = [&](const proto::Frame& frame, TransferSession& receiver,
                     const std::string& name) {
    auto handled = receiver.Handle(RoundTrip(frame), now);
    CHECK(handled.has_value()) << handled.error();
    TransferEvents events = std::move(*handled);
    for (size_t i = 0; i < events.size(); ++i) {
      if (const auto* offer = std::get_if<OfferReceived>(&events[i])) {
        auto responded = receiver.Respond(offer->offer.transfer_id(), decision,
                                          accepted_ids);
        CHECK(responded.has_value()) << responded.error();
        for (TransferEvent& event : *responded) {
          events.push_back(std::move(event));
        }
      } else if (const auto* finished =
                     std::get_if<TransferFinished>(&events[i])) {
        run.results[name] = finished->result;
      }
    }
  };

  struct Direction {
    raw_ref<TransferSession> sender;
    raw_ref<TransferSession> receiver;
    std::string name;
    std::string other;
  };
  const Direction directions[] = {
      {raw_ref(responder), raw_ref(initiator), "responder", "initiator"},
      {raw_ref(initiator), raw_ref(responder), "initiator", "responder"},
  };
  bool progressed = true;
  while (progressed) {
    progressed = false;
    for (const Direction& direction : directions) {
      while (true) {
        auto frame = direction.sender->NextFrame(now);
        CHECK(frame.has_value()) << frame.error();
        if (!frame->has_value()) {
          break;
        }
        run.frames.emplace_back(direction.name, **frame);
        deliver(**frame, *direction.receiver, direction.other);
        progressed = true;
      }
    }
  }
  EXPECT_TRUE(initiator.IsIdle());
  EXPECT_TRUE(responder.IsIdle());
  return run;
}

std::optional<int> OptionalInt(const base::DictValue& dict,
                               std::string_view key) {
  const base::Value* value = dict.Find(key);
  if (!value || value->is_none()) {
    return std::nullopt;
  }
  return value->GetInt();
}

}  // namespace

TEST(BraveTransferSessionTest, TranscriptVectors) {
  base::DictValue vectors = test::LoadVectors("session_transcript.json");
  const Bytes transfer_id = test::FromHex(*vectors.FindString("transfer_id"));
  const base::ListValue* scenarios = vectors.FindList("scenarios");
  ASSERT_TRUE(scenarios);
  ASSERT_EQ(scenarios->size(), 4u);
  for (const base::Value& entry : *scenarios) {
    const base::DictValue& scenario = entry.GetDict();
    SCOPED_TRACE(*scenario.FindString("name"));
    std::vector<uint32_t> accepted_ids;
    for (const base::Value& id : *scenario.FindList("accepted_item_ids")) {
      accepted_ids.push_back(static_cast<uint32_t>(id.GetInt()));
    }
    ScenarioRun run =
        RunScenario(test::TranscriptItems(vectors), transfer_id,
                    static_cast<proto::Decision>(*scenario.FindInt("decision")),
                    accepted_ids);

    const base::ListValue* expected = scenario.FindList("frames");
    ASSERT_EQ(run.frames.size(), expected->size());
    for (size_t i = 0; i < run.frames.size(); ++i) {
      SCOPED_TRACE(i);
      const base::DictValue& want = (*expected)[i].GetDict();
      const Bytes raw = EncodeMessage(run.frames[i].second);
      EXPECT_EQ(run.frames[i].first, *want.FindString("sender"));
      EXPECT_EQ(raw.size(), static_cast<size_t>(*want.FindInt("length")));
      EXPECT_EQ(test::HexString(crypto::hash::Sha256(raw)),
                *want.FindString("sha256"));
      if (const std::string* plaintext = want.FindString("plaintext")) {
        EXPECT_EQ(test::HexString(raw), *plaintext);
      }
    }

    for (const char* side : {"initiator", "responder"}) {
      SCOPED_TRACE(side);
      const base::DictValue& want =
          *scenario.FindDict(std::string(side) + "_result");
      ASSERT_TRUE(run.results.contains(side));
      const TransferResult& got = run.results[side];
      EXPECT_EQ(test::HexString(got.transfer_id), test::HexString(transfer_id));
      EXPECT_EQ(got.is_outgoing, std::string(side) == "initiator");
      std::optional<int> status = OptionalInt(want, "status");
      std::optional<int> cancel_reason = OptionalInt(want, "cancel_reason");
      std::optional<int> decision = OptionalInt(want, "decision");
      EXPECT_EQ(got.status.has_value(), status.has_value());
      if (got.status && status) {
        EXPECT_EQ(static_cast<int>(*got.status), *status);
      }
      EXPECT_EQ(got.cancel_reason.has_value(), cancel_reason.has_value());
      EXPECT_EQ(got.decision.has_value(), decision.has_value());
      if (got.decision && decision) {
        EXPECT_EQ(static_cast<int>(*got.decision), *decision);
      }
    }
  }
}

TEST(BraveTransferSessionTest, ChunksReassemble) {
  TransferSession sender(kZero);
  TransferSession receiver(kZero);
  const std::string id = Offer(sender, receiver);
  ASSERT_TRUE(receiver.Respond(id, proto::DECISION_ACCEPT_ALL).has_value());
  MustDeliver(Drain(receiver), sender);

  std::map<uint32_t, Bytes> data;
  base::flat_set<uint32_t> received;
  for (const TransferEvent& event : MustDeliver(Drain(sender), receiver)) {
    if (const auto* chunk = std::get_if<ChunkReceived>(&event)) {
      data[chunk->item_id].insert(data[chunk->item_id].end(),
                                  chunk->data.begin(), chunk->data.end());
    } else if (const auto* item = std::get_if<ItemReceived>(&event)) {
      received.insert(item->item.item_id());
    }
  }
  EXPECT_EQ(data[3], test::Pattern(62440));
  EXPECT_EQ(received, (base::flat_set<uint32_t>{3, 5}));
  std::vector<TransferResult> results =
      Finished(MustDeliver(Drain(receiver), sender));
  ASSERT_EQ(results.size(), 1u);
  EXPECT_EQ(results[0].status, proto::DONE_STATUS_OK);
  EXPECT_TRUE(sender.IsIdle());
  EXPECT_TRUE(receiver.IsIdle());
}

TEST(BraveTransferSessionTest, SecondIncomingOfferIsBusy) {
  TransferSession a(kZero);
  TransferSession b(kZero);
  TransferSession c(kZero);
  Offer(a, b);
  auto id = c.SendOffer(VectorItems(), kZero);
  ASSERT_TRUE(id.has_value());
  EXPECT_TRUE(MustDeliver(Drain(c), b).empty());
  std::vector<proto::Frame> frames = Drain(b);
  ASSERT_EQ(frames.size(), 1u);
  EXPECT_EQ(Encoded(frames[0]),
            Encoded(ResponseFrame(*id, proto::DECISION_BUSY)));
  std::vector<TransferResult> results = Finished(MustDeliver(frames, c));
  ASSERT_EQ(results.size(), 1u);
  EXPECT_EQ(results[0].decision, proto::DECISION_BUSY);
  EXPECT_TRUE(b.HasIncomingTransfer());
}

TEST(BraveTransferSessionTest, HashMismatch) {
  OutgoingItem bad = OutgoingItem::FromBytes(proto::ITEM_KIND_FILE, "f", "",
                                             Bytes(100000, 0x78));
  const uint8_t one[] = {1};
  bad.sha256 = base::ToVector(crypto::hash::Sha256(one));
  TransferSession s(kZero);
  TransferSession r(kZero);
  std::vector<OutgoingItem> items;
  items.push_back(std::move(bad));
  const std::string id = Offer(s, r, std::move(items));
  ASSERT_TRUE(r.Respond(id, proto::DECISION_ACCEPT_ALL).has_value());
  MustDeliver(Drain(r), s);
  std::vector<TransferResult> receiver_results =
      Finished(MustDeliver(Drain(s), r));
  ASSERT_EQ(receiver_results.size(), 1u);
  EXPECT_EQ(receiver_results[0].status, proto::DONE_STATUS_HASH_MISMATCH);
  std::vector<TransferResult> sender_results =
      Finished(MustDeliver(Drain(r), s));
  ASSERT_EQ(sender_results.size(), 1u);
  EXPECT_EQ(sender_results[0].status, proto::DONE_STATUS_HASH_MISMATCH);
}

TEST(BraveTransferSessionTest, WriteFailure) {
  TransferSession s(kZero);
  TransferSession r(kZero);
  const std::string id = Offer(s, r);
  ASSERT_TRUE(r.Respond(id, proto::DECISION_ACCEPT_ALL).has_value());
  MustDeliver(Drain(r), s);
  auto first = s.NextFrame(kZero);
  ASSERT_TRUE(first.has_value() && first->has_value());
  MustDeliver({**first}, r);

  auto failed = r.WriteFailed(id);
  ASSERT_TRUE(failed.has_value());
  std::vector<TransferResult> receiver_results = Finished(*failed);
  ASSERT_EQ(receiver_results.size(), 1u);
  EXPECT_EQ(receiver_results[0].status, proto::DONE_STATUS_WRITE_FAILED);
  std::vector<TransferResult> sender_results =
      Finished(MustDeliver(Drain(r), s));
  ASSERT_EQ(sender_results.size(), 1u);
  EXPECT_EQ(sender_results[0].status, proto::DONE_STATUS_WRITE_FAILED);
  EXPECT_TRUE(s.IsIdle());
  EXPECT_TRUE(r.IsIdle());
}

TEST(BraveTransferSessionTest, CancelMidTransferIgnoresFramesInFlight) {
  TransferSession s(kZero);
  TransferSession r(kZero);
  const std::string id = Offer(s, r);
  ASSERT_TRUE(r.Respond(id, proto::DECISION_ACCEPT_ALL).has_value());
  MustDeliver(Drain(r), s);
  auto first = s.NextFrame(kZero);
  ASSERT_TRUE(first.has_value() && first->has_value());
  const proto::Frame first_chunk = **first;

  std::vector<TransferResult> cancelled = Finished(s.Cancel(id));
  ASSERT_EQ(cancelled.size(), 1u);
  EXPECT_EQ(cancelled[0].cancel_reason, proto::CANCEL_REASON_USER);
  std::vector<proto::Frame> rest = Drain(s);
  ASSERT_EQ(rest.size(), 1u);
  EXPECT_EQ(Encoded(rest[0]),
            Encoded(CancelFrame(id, proto::CANCEL_REASON_USER)));

  std::vector<proto::Frame> in_flight = {first_chunk, rest[0]};
  std::vector<TransferResult> results = Finished(MustDeliver(in_flight, r));
  ASSERT_EQ(results.size(), 1u);
  EXPECT_EQ(results[0].cancel_reason, proto::CANCEL_REASON_USER);
  EXPECT_TRUE(r.IsIdle());
  EXPECT_TRUE(s.IsIdle());
  EXPECT_TRUE(MustDeliver({first_chunk}, r).empty());
}

TEST(BraveTransferSessionTest, Timeouts) {
  TransferSession s(kZero);
  TransferSession r(kZero);
  const std::string id = Offer(s, r);
  const base::TimeTicks t = At(kOfferTimeout + base::Seconds(1));
  ASSERT_TRUE(s.Handle(PingFrame(1), t).has_value());
  ASSERT_TRUE(r.Handle(PingFrame(1), t).has_value());

  auto sender_poll = s.Poll(t);
  ASSERT_TRUE(sender_poll.has_value());
  std::vector<TransferResult> sender_results = Finished(*sender_poll);
  ASSERT_EQ(sender_results.size(), 1u);
  EXPECT_EQ(sender_results[0].cancel_reason, proto::CANCEL_REASON_TIMEOUT);
  std::vector<proto::Frame> frames = Drain(s, t);
  ASSERT_FALSE(frames.empty());
  EXPECT_EQ(Encoded(frames[0]),
            Encoded(CancelFrame(id, proto::CANCEL_REASON_TIMEOUT)));

  auto receiver_poll = r.Poll(t);
  ASSERT_TRUE(receiver_poll.has_value());
  std::vector<TransferResult> receiver_results = Finished(*receiver_poll);
  ASSERT_EQ(receiver_results.size(), 1u);
  EXPECT_EQ(receiver_results[0].decision, proto::DECISION_DECLINE);

  TransferSession idle(kZero);
  auto early = idle.Poll(At(kPingInterval - base::Seconds(1)));
  ASSERT_TRUE(early.has_value());
  EXPECT_TRUE(early->empty());
  EXPECT_TRUE(Drain(idle).empty());
  ASSERT_TRUE(idle.Poll(At(kPingInterval)).has_value());
  std::vector<proto::Frame> pings = Drain(idle, At(kPingInterval));
  ASSERT_EQ(pings.size(), 1u);
  EXPECT_EQ(Encoded(pings[0]), Encoded(PingFrame(1)));
  EXPECT_FALSE(
      idle.Poll(At(kIdleTimeout + base::Milliseconds(500))).has_value());
}

TEST(BraveTransferSessionTest, ResponderSendsConfirmationPing) {
  TransferSession responder(kZero, ReceiveLimits(),
                            /*send_confirmation=*/true);
  std::vector<proto::Frame> frames = Drain(responder);
  ASSERT_EQ(frames.size(), 1u);
  EXPECT_EQ(Encoded(frames[0]), Encoded(PingFrame(0)));
}

TEST(BraveTransferSessionTest, BadChunksAreProtocolErrors) {
  const std::vector<base::RepeatingCallback<void(proto::Chunk&)>> mutations = {
      base::BindRepeating(
          [](proto::Chunk& c) { c.set_offset(c.offset() + 1); }),
      base::BindRepeating([](proto::Chunk& c) { c.set_item_id(99); }),
      base::BindRepeating([](proto::Chunk& c) { c.clear_data(); }),
      base::BindRepeating([](proto::Chunk& c) {
        c.set_transfer_id(std::string(kTransferIdSize, '\0'));
      }),
  };
  for (size_t i = 0; i < mutations.size(); ++i) {
    SCOPED_TRACE(i);
    TransferSession s(kZero);
    TransferSession r(kZero);
    const std::string id = Offer(s, r);
    ASSERT_TRUE(r.Respond(id, proto::DECISION_ACCEPT_ALL).has_value());
    MustDeliver(Drain(r), s);
    auto frame = s.NextFrame(kZero);
    ASSERT_TRUE(frame.has_value() && frame->has_value());
    proto::Frame chunk = **frame;
    ASSERT_TRUE(chunk.has_chunk());
    mutations[i].Run(*chunk.mutable_chunk());
    EXPECT_FALSE(Deliver({chunk}, r).has_value());
  }
}

TEST(BraveTransferSessionTest, BadResponsesAreProtocolErrors) {
  const std::vector<std::pair<proto::Decision, std::vector<uint32_t>>>
      responses = {
          {proto::DECISION_ACCEPT_ALL, {1}},
          {proto::DECISION_ACCEPT_SOME, {}},
          {proto::DECISION_ACCEPT_SOME, {1, 1}},
          {proto::DECISION_ACCEPT_SOME, {42}},
          {proto::DECISION_DECLINE, {1}},
          {proto::DECISION_UNSPECIFIED, {}},
      };
  for (const auto& [decision, ids] : responses) {
    SCOPED_TRACE(static_cast<int>(decision));
    TransferSession s(kZero);
    TransferSession r(kZero);
    const std::string id = Offer(s, r);
    EXPECT_FALSE(s.Handle(ResponseFrame(id, decision, ids), kZero).has_value());
  }
}

TEST(BraveTransferSessionTest, ReusedTransferIdIsAnError) {
  TransferSession a(kZero, ReceiveLimits(), /*send_confirmation=*/false,
                    FixedRandom(Bytes(kTransferIdSize, 1)));
  TransferSession b(kZero);
  const std::string id = Offer(a, b);
  ASSERT_TRUE(b.Respond(id, proto::DECISION_DECLINE).has_value());
  MustDeliver(Drain(b), a);
  ASSERT_TRUE(a.SendOffer(VectorItems(), kZero).has_value());
  EXPECT_FALSE(Deliver(Drain(a), b).has_value());
}

TEST(BraveTransferSessionTest, OneOutgoingTransferAtATime) {
  TransferSession s(kZero);
  ASSERT_TRUE(s.SendOffer(VectorItems(), kZero).has_value());
  EXPECT_FALSE(s.SendOffer(VectorItems(), kZero).has_value());
  EXPECT_TRUE(s.HasOutgoingTransfer());
  EXPECT_FALSE(s.IsIdle());
}

TEST(BraveTransferSessionTest, ReadFailureIsAnError) {
  OutgoingItem item = OutgoingItem::FromBytes(proto::ITEM_KIND_FILE, "f.bin",
                                              "", test::Pattern(1000));
  item.read = base::BindRepeating(
      [](uint64_t, size_t) -> std::optional<Bytes> { return std::nullopt; });
  TransferSession s(kZero);
  TransferSession r(kZero);
  std::vector<OutgoingItem> items;
  items.push_back(std::move(item));
  const std::string id = Offer(s, r, std::move(items));
  ASSERT_TRUE(r.Respond(id, proto::DECISION_ACCEPT_ALL).has_value());
  MustDeliver(Drain(r), s);
  EXPECT_FALSE(s.NextFrame(kZero).has_value());
}

}  // namespace brave_transfer
