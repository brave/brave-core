#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_COMMON_TO_VALUE_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_COMMON_TO_VALUE_H_

#include <vector>

#include "base/containers/flat_map.h"
#include "base/values.h"
#include "brave/components/brave_news/common/brave_news.mojom-forward.h"

namespace brave_news::mojom {

template <typename T>
base::Value ToValue(const base::flat_map<std::string, T>& value);

template <typename T>
base::Value ToValue(const std::vector<T>& value);

base::Value ToValue(const StatePtr& value);

base::Value ToValue(const ConfigurationPtr& value);

base::Value ToValue(const ChannelPtr& value);

base::Value ToValue(const PublisherPtr& value);

}  // namespace brave_news::mojom

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_COMMON_TO_VALUE_H_
