// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <ios/web/webui/web_ui_ios_data_source_impl.mm>

namespace web {

WebUIIOSDataSourceImpl::WebUIIOSDataSourceImpl(std::string_view source_name,
                                               URLDataSourceIOS* source)
    : URLDataSourceIOSImpl(source_name, source),
      source_name_(source_name),
      default_resource_(-1),
      deny_xframe_options_(true),
      load_time_data_defaults_added_(false),
      replace_existing_source_(true),
      should_replace_i18n_in_js_(false) {}

}  // namespace web
