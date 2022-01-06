/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_BAT_LEDGER_CONTEXT_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_BAT_LEDGER_CONTEXT_H_

#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

#include "base/bind.h"
#include "base/location.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/supports_user_data.h"

namespace ledger {

class LedgerImpl;
class LedgerClient;

// Represents a running instance of the BAT client engine. It serves as a
// loosely-coupled container for services, jobs and other components that are
// associated with, and owned by, the running instance. It also provides methods
// for logging and accessing start-up options.
class BATLedgerContext : private base::SupportsUserData {
 public:
  // Base class for objects that are owned by an instance of BATLedgerContext.
  // The lifetime of the object is bounded by the lifetime of the context. As
  // such, it is generally safe for an object to access its containing context
  // by calling the |context()| accessor. Access to the containing context is
  // not allowed from the object's constructor or destructor.
  //
  // There are currently two primary object subtypes: components and jobs.
  // Components define a static |kContextKey| member. Instances are
  // initialized when first requested and are destroyed when the context is
  // destroyed.
  //
  //   class MyComponent : public BATLedgerContext::Object {
  //    public:
  //     inline static const char kContextKey[] = "my-component";
  //     ...
  //   };
  //
  // Job classes logically represent a single asynchronous operation and are
  // destroyed automatically when the operation completes.
  //
  //   class MyJob : public BATLedgerContext::Object {
  //    public:
  //     Future<int> GetFuture() { ... }
  //     void Start() { ... }
  //   };
  //
  // Jobs must implement a public |GetFuture| method that returns an Future
  // object, and a |Start| method that begins the asynchronous operation.
  class Object : public base::SupportsUserData::Data {
   public:
    Object(const Object&) = delete;
    Object& operator=(const Object&) = delete;

    ~Object() override;

   protected:
    Object();

    BATLedgerContext& context() const { return *context_; }

   private:
    friend class BATLedgerContext;

    base::WeakPtr<BATLedgerContext> context_;
  };

  enum class Environment { kDevelopment, kStaging, kProduction };

  struct Options {
    Environment environment = Environment::kProduction;
    bool auto_contribute_allowed = false;
    bool enable_experimental_features = false;
  };

  // NOTE: Values are based on the original logging design where each level from
  // 0 to 9 were assigned a specific subject matter.
  enum class LogLevel { kError = 0, kInfo = 1, kVerbose = 6, kFull = 9 };

  // Helper stream class returned from BATLedgerContext::Log* methods. Upon
  // destruction, the resulting string is sent to the client for logging.
  class LogStream {
   public:
    LogStream(base::WeakPtr<BATLedgerContext> context,
              base::Location location,
              LogLevel log_level);

    LogStream(const LogStream&) = delete;
    LogStream& operator=(const LogStream&) = delete;

    LogStream(LogStream&& other);
    LogStream& operator=(LogStream&& other);

    ~LogStream();

    template <typename T>
    LogStream& operator<<(T&& value) {
      stream_ << std::forward<T>(value);
      return *this;
    }

   private:
    base::WeakPtr<BATLedgerContext> context_;
    base::Location location_;
    LogLevel log_level_;
    std::ostringstream stream_;
  };

  explicit BATLedgerContext(LedgerClient* ledger_client);
  explicit BATLedgerContext(LedgerImpl* ledger_impl);

  ~BATLedgerContext() override;

  BATLedgerContext(const BATLedgerContext&) = delete;
  BATLedgerContext& operator=(const BATLedgerContext&) = delete;

  // Returns the startup options for this ledger context.
  const Options& options() const { return options_; }

  // Returns a pointer to the LedgerClient associated with this ledger context.
  // In general, this method should only be used by low-level components that
  // interact directly with the client. Before using this function in an
  // application-level component, check to see if there is an existing component
  // that wraps the low-level calls, and if not, consider creating one.
  LedgerClient* GetLedgerClient() const { return ledger_client_; }

  // Returns a pointer to the LedgerImpl associated with this ledger context.
  // This method is provided for backward-compatibility with existing code that
  // does not expose a context component and will be removed in the future.
  LedgerImpl* GetLedgerImpl() const { return ledger_impl_; }

  // Returns a reference to the context component of type T. T must expose a
  // static |kContextKey| member. The object will be created if necessary.
  //
  // Example:
  //   auto& my_component = context().Get<MyComponent>();
  template <typename T>
  T& Get() {
    static_assert(std::is_base_of<Object, T>::value,
                  "Get<T> requires that T is a subclass of "
                  "BATLedgerContext::Object");
    const void* key = T::kContextKey;
    if (T* ptr = static_cast<T*>(this->GetUserData(key))) {
      return *ptr;
    }

    auto instance = std::make_unique<T>();
    T* ptr = instance.get();
    ptr->context_ = weak_factory_.GetWeakPtr();
    this->SetUserData(key, std::move(instance));
    return *ptr;
  }

  template <typename T>
  void SetComponentForTesting(std::unique_ptr<T> instance) {
    static_assert(std::is_base_of<Object, T>::value,
                  "SetComponentForTesting<T> requires that T is a subclass of "
                  "BATLedgerContext::Object");
    instance->context_ = weak_factory_.GetWeakPtr();
    this->SetUserData(T::kContextKey, std::move(instance));
  }

  // Starts a job and returns the Future associated with it. When the job
  // completes, the job instance is destroyed.
  //
  // Example:
  //   auto future = context().StartJob<MyJob>("hello");
  //   future.Then(...);
  template <typename T, typename... Args>
  auto StartJob(Args&&... args) {
    static_assert(std::is_base_of<Object, T>::value,
                  "StartJob<T, Args...> requires that T is a subclass of "
                  "BATLedgerContext::Object");
    auto instance = std::make_unique<T>();
    T* ptr = instance.get();
    this->SetUserData(ptr, std::move(instance));

    ptr->context_ = weak_factory_.GetWeakPtr();
    ptr->Start(std::forward<Args>(args)...);
    auto future = ptr->GetFuture();

    using ValueType = typename decltype(future)::ValueType;
    return future.Then(
        base::BindOnce(BATLedgerContext::OnJobCompleted<ValueType>,
                       weak_factory_.GetWeakPtr(), ptr));
  }

  // The Log* functions return a LogStream used to log messages to the client.
  // Log levels kError, kInfo, and kVerbose may be logged to disk by the client
  // and should not contain any information that would result in a breach of
  // security.
  //
  // Example:
  //   context().LogError(FROM_HERE) << "Something didn't work quite right";
  //
  // Since non-temporary objects are destroyed in LIFO order, be careful when
  // binding a LogStream to an lvalue, as it could result in a surprising
  // ordering of logging calls to the client.
  //
  //   LogStream stream1 = context().LogInfo(FROM_HERE);
  //   stream1 << "A";
  //   LogStream stream2 = context().LogInfo(FROM_HERE);
  //   stream2 << "B";
  //
  // In the example above, |stream2| is destroyed first, and therefore sends its
  // message to the client first.
  LogStream Log(base::Location location, LogLevel log_level);
  LogStream LogError(base::Location location);
  LogStream LogInfo(base::Location location);
  LogStream LogVerbose(base::Location location);
  LogStream LogFull(base::Location location);

  base::WeakPtr<BATLedgerContext> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

 private:
  template <typename T>
  static T OnJobCompleted(base::WeakPtr<BATLedgerContext> context,
                          void* job_key,
                          T value) {
    if (context) {
      // Invalidate the job's context pointer before executing its destructor.
      if (auto* ptr = static_cast<Object*>(context->GetUserData(job_key))) {
        ptr->context_ = nullptr;
      }
      context->RemoveUserData(job_key);
    }

    return value;
  }

  raw_ptr<LedgerClient> ledger_client_{nullptr};
  raw_ptr<LedgerImpl> ledger_impl_{nullptr};
  Options options_;
  base::WeakPtrFactory<BATLedgerContext> weak_factory_{this};
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_BAT_LEDGER_CONTEXT_H_
