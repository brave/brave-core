/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_DE_AMP_BROWSER_DE_AMP_URL_LOADER_H_
#define BRAVE_COMPONENTS_DE_AMP_BROWSER_DE_AMP_URL_LOADER_H_

#include <string>
#include <tuple>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/body_sniffer/body_sniffer_url_loader.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "url/gurl.h"

namespace de_amp {

class DeAmpThrottle;

class DeAmpURLLoader : public body_sniffer::BodySnifferURLLoader {
 public:
  ~DeAmpURLLoader() override;
  DeAmpURLLoader& operator=(const DeAmpURLLoader&) = delete;

  // mojo::PendingRemote<network::mojom::URLLoader> controls the lifetime of the
  // loader.
  static std::tuple<mojo::PendingRemote<network::mojom::URLLoader>,
                    mojo::PendingReceiver<network::mojom::URLLoaderClient>,
                    DeAmpURLLoader*>
  CreateLoader(base::WeakPtr<DeAmpThrottle> throttle,
               const GURL& response_url,
               scoped_refptr<base::SequencedTaskRunner> task_runner);

 private:
  DeAmpURLLoader(base::WeakPtr<DeAmpThrottle> throttle,
                 const GURL& response_url,
                 mojo::PendingRemote<network::mojom::URLLoaderClient>
                     destination_url_loader_client,
                 scoped_refptr<base::SequencedTaskRunner> task_runner);

  void OnBodyReadable(MojoResult) override;
  void OnBodyWritable(MojoResult) override;

  bool MaybeRedirectToCanonicalLink();

  void ForwardBodyToClient();

  base::WeakPtr<DeAmpThrottle> de_amp_throttle_;
};

}  // namespace de_amp

#endif  // BRAVE_COMPONENTS_DE_AMP_BROWSER_DE_AMP_URL_LOADER_H_
