// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <ios/web/web_state/web_state_impl.mm>

namespace web {

WebUIIOS* WebStateImpl::GetMainFrameWebUI() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return RealizedState()->GetMainFrameWebUI();
}

size_t WebStateImpl::GetWebUICountForTesting() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return RealizedState()->GetWebUICountForTesting();  // IN-TEST
}

}  // namespace web
