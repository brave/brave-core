// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_SKUS_BROWSER_SKUS_CONTEXT_IMPL_H_
#define BRAVE_COMPONENTS_SKUS_BROWSER_SKUS_CONTEXT_IMPL_H_

#include <memory>
#include <string>

#include "base/memory/raw_ref.h"
#include "base/memory/scoped_refptr.h"
#include "brave/components/skus/browser/rs/cxx/src/shim.h"
#include "brave/components/skus/browser/skus_service_impl.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace skus {
class SkusUrlLoader;
}  // namespace skus

namespace skus {

// Context object used with the SKU SDK to provide 1) key/value pair storage
// and 2) the url loader used for contacting the SKU SDK endpoint via HTTPS.
//
// In the .cc, there are implementations for global methods originally defined
// in `rs/cxx/src/shim.h`. These implementations are called from
// Rust and will pass this context object along, so that the results can be
// persisted.
class SkusContextImpl : public SkusContext {
 public:
  SkusContextImpl(const SkusContextImpl&) = delete;
  SkusContextImpl& operator=(const SkusContextImpl&) = delete;

  explicit SkusContextImpl(
      std::unique_ptr<network::PendingSharedURLLoaderFactory>
          pending_url_loader_factory,
      scoped_refptr<base::SequencedTaskRunner> ui_task_runner,
      base::WeakPtr<SkusServiceImpl>);
  ~SkusContextImpl() override;

  std::unique_ptr<skus::SkusUrlLoader> CreateFetcher() const override;
  void GetValueFromStore(
      const std::string& key,
      rust::cxxbridge1::Fn<void(rust::cxxbridge1::Box<skus::StorageGetContext>,
                                rust::String value,
                                bool success)> done,
      rust::cxxbridge1::Box<skus::StorageGetContext> st_ctx) const override;
  void PurgeStore(
      rust::cxxbridge1::Fn<
          void(rust::cxxbridge1::Box<skus::StoragePurgeContext>, bool success)>
          done,
      rust::cxxbridge1::Box<skus::StoragePurgeContext> st_ctx) const override;
  void UpdateStoreValue(
      const std::string& key,
      const std::string& value,
      rust::cxxbridge1::Fn<void(rust::cxxbridge1::Box<skus::StorageSetContext>,
                                bool success)> done,
      rust::cxxbridge1::Box<skus::StorageSetContext> st_ctx) const override;

 private:
  SEQUENCE_CHECKER(sequence_checker_);
  // used for making requests to SKU server
  mutable std::unique_ptr<network::PendingSharedURLLoaderFactory>
      pending_url_loader_factory_ GUARDED_BY_CONTEXT(sequence_checker_);
  scoped_refptr<base::SequencedTaskRunner> ui_task_runner_;
  base::WeakPtr<SkusServiceImpl> skus_service_;
};

}  // namespace skus

#endif  // BRAVE_COMPONENTS_SKUS_BROWSER_SKUS_CONTEXT_IMPL_H_
