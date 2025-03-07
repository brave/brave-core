#include "src/ios/web/webui/web_ui_ios_data_source_impl.mm"

namespace web {

WebUIIOSDataSourceImpl::WebUIIOSDataSourceImpl(const std::string& source_name,
                                               URLDataSourceIOS* source)
    : URLDataSourceIOSImpl(source_name, source),
      source_name_(source_name),
      default_resource_(-1),
      deny_xframe_options_(true),
      load_time_data_defaults_added_(false),
      replace_existing_source_(true),
      should_replace_i18n_in_js_(false) {}

}  // namespace web
