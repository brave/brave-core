#include "brave/components/brave_news/common/to_value.h"

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/values.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "url/gurl.h"

namespace brave_news::mojom {

namespace {
base::Value ToValue(const std::string& value) {
  return base::Value(value);
}

base::Value ToValue(const GURL& value) {
  auto dict = base::Value::Dict();
  dict.Set("url", value.spec());
  return base::Value(std::move(dict));
}

template <typename T>
base::Value ToValue(const std::optional<T>& value) {
  if (!value) {
    return base::Value(base::Value::Type::NONE);
  }
  return base::Value(ToValue(value.value()));
}
}  // namespace


template <typename T>
base::Value ToValue(const base::flat_map<std::string, T>& value) {
  auto dict = base::Value::Dict();
  for (const auto& item : value) {
    dict.Set(item.first, ToValue(item.second));
  }
  return base::Value(std::move(dict));
}

template <typename T>
base::Value ToValue(const std::vector<T>& value) {
  auto list = base::Value::List();
  for (const auto& item : value) {
    list.Append(ToValue(item));
  }
  return base::Value(std::move(list));
}
base::Value ToValue(const LocaleInfoPtr& value) {
  if (!value) {
    return base::Value(base::Value::Type::NONE);
  }

  auto dict = base::Value::Dict();
  dict.Set("locale", value->locale);
  dict.Set("rank", static_cast<int>(value->rank));
  dict.Set("channels", ToValue(value->channels));
  return base::Value(std::move(dict));
}

base::Value ToValue(const StatePtr& value) {
  if (!value) {
    return base::Value(base::Value::Type::NONE);
  }

  auto dict = base::Value::Dict();
  dict.Set("channels", ToValue(value->channels));
  dict.Set("publishers", ToValue(value->publishers));
  dict.Set("configuration", ToValue(value->configuration));
  return base::Value(std::move(dict));
}

base::Value ToValue(const ConfigurationPtr& value) {
  if (!value) {
    return base::Value(base::Value::Type::NONE);
  }
  auto dict = base::Value::Dict();
  dict.Set("isOptedIn", value->isOptedIn);
  dict.Set("showOnNTP", value->showOnNTP);
  dict.Set("openArticlesInNewTab", value->openArticlesInNewTab);
  return base::Value(std::move(dict));
}

base::Value ToValue(const ChannelPtr& value) {
  if (!value) {
    return base::Value(base::Value::Type::NONE);
  }

  auto dict = base::Value::Dict();
  dict.Set("channelName", value->channel_name);
  dict.Set("subscribedLocales", ToValue(value->subscribed_locales));

  return base::Value(std::move(dict));
}

base::Value ToValue(const PublisherPtr& value) {
  if (!value) {
    return base::Value(base::Value::Type::NONE);
  }

  auto dict = base::Value::Dict();
  dict.Set("publisherId", value->publisher_id);
  dict.Set("type", static_cast<int>(value->type));
  dict.Set("publisherName", value->publisher_name);
  dict.Set("categoryName", value->category_name);
  dict.Set("isEnabled", value->is_enabled);
  dict.Set("locales", ToValue(value->locales));
  dict.Set("feedSource", ToValue(value->feed_source));
  dict.Set("faviconUrl", ToValue(value->favicon_url));
  dict.Set("coverUrl", ToValue(value->cover_url));
  dict.Set("backgroundColor", ToValue(value->background_color));
  dict.Set("siteUrl", ToValue(value->site_url));
  dict.Set("userEnabledStatus", static_cast<int>(value->user_enabled_status));
  return base::Value(std::move(dict));
}

}  // namespace brave_news::mojom
