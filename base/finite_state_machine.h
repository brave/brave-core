/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BASE_FINITE_STATE_MACHINE_H_
#define BRAVE_BASE_FINITE_STATE_MACHINE_H_

#include <concepts>
#include <memory>
#include <type_traits>
#include <utility>

#include "base/check.h"
#include "base/compiler_specific.h"

namespace brave::internal {

class TypeId final {
 public:
  bool operator==(TypeId other) const { return id_ == other.id_; }

  template <typename T>
  static TypeId For() {
    return TypeId(&For<T>);
  }

 private:
  using Id = TypeId (*)();

  explicit TypeId(Id id) : id_(id) {}

  Id id_;
};

struct MessageBase {
  virtual TypeId GetTypeId() const = 0;
  virtual ~MessageBase() = default;
};

template <typename Message>
class MessageWrapper final : public MessageBase {
 public:
  template <typename M>
  explicit MessageWrapper(M&& message) : message_(std::forward<M>(message)) {}

  Message&& operator*() && { return std::move(message_); }

 private:
  TypeId GetTypeId() const override { return TypeId::For<Message>(); }

  Message message_;
};

template <typename M>
concept Message = !std::same_as<M, void> && std::same_as<M, std::decay_t<M>>;

template <typename H, typename M>
concept Handler = std::invocable<H, M> || std::invocable<H>;

template <typename Handler = void,
          typename Message = void,
          typename PreviousDispatcher = void>
class Dispatcher {
  static_assert(std::is_void_v<Handler> == std::is_void_v<Message> &&
                std::is_void_v<Message> == std::is_void_v<PreviousDispatcher>);

  static constexpr bool kIsSink = std::is_void_v<Handler>;

 public:
  explicit Dispatcher(std::unique_ptr<MessageBase> message)
    requires(kIsSink)
      : message_(std::move(message)) {}

  template <typename H>
  Dispatcher(std::unique_ptr<MessageBase> message,
             H&& handler,
             const PreviousDispatcher* previous_dispatcher)
    requires(!kIsSink)
      : message_(std::move(message)),
        handler_(std::forward<H>(handler)),
        previous_dispatcher_(previous_dispatcher) {}

  ~Dispatcher() noexcept(false) {
    if (message_) {
      Dispatch(std::move(message_));
    }
  }

  template <internal::Message M, internal::Handler<M&&> H>
  auto message(H&& handler) {
    return Dispatcher<std::decay_t<H>, M, Dispatcher>(
        std::move(message_), std::forward<H>(handler), this);
  }

 private:
  template <typename, typename, typename>
  friend class Dispatcher;

  void Dispatch(std::unique_ptr<MessageBase> message) const {
    if constexpr (!kIsSink) {
      if (message->GetTypeId() != TypeId::For<Message>()) {
        return previous_dispatcher_->Dispatch(std::move(message));
      }

      if constexpr (std::invocable<Handler>) {
        handler_();
      } else {
        handler_(*std::move(static_cast<MessageWrapper<Message>&>(*message)));
      }
    }
  }

  std::unique_ptr<MessageBase> message_;

  struct Empty {};
  NO_UNIQUE_ADDRESS std::conditional_t<kIsSink, Empty, Handler> handler_;
  NO_UNIQUE_ADDRESS
  std::conditional_t<kIsSink, Empty, const PreviousDispatcher*>
      previous_dispatcher_;
};

}  // namespace brave::internal

namespace brave {

template <typename T>
class FiniteStateMachine {
 public:
  template <typename Message>
  void Send(Message&& message) {
    message_ =
        std::make_unique<internal::MessageWrapper<std::decay_t<Message>>>(
            std::forward<Message>(message));
    (static_cast<T*>(this)->*state_)();
    CHECK(!message_);
  }

 protected:
  using State = void (T::*)();

  explicit FiniteStateMachine(State state) : state_(state) {}

  auto handle() { return internal::Dispatcher(std::move(message_)); }

  State state_;

 private:
  std::unique_ptr<internal::MessageBase> message_;
};

}  // namespace brave

#endif  // BRAVE_BASE_FINITE_STATE_MACHINE_H_
