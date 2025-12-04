// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/oai_parsing.h"

#include <string>
#include <utility>

#include "base/logging.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "url/gurl.h"

namespace ai_chat {

std::vector<mojom::ToolUseEventPtr> ToolUseEventFromToolCallsResponse(
    const base::Value::List* tool_calls_api_response) {
  // https://platform.openai.com/docs/api-reference/chat/create#chat-create-tools
  // https://platform.openai.com/docs/api-reference/chat/object
  // choices -> message -> tool_calls
  std::vector<mojom::ToolUseEventPtr> tool_use_events;
  for (auto& tool_call_raw : *tool_calls_api_response) {
    if (!tool_call_raw.is_dict()) {
      DLOG(ERROR) << "Tool call is not a dictionary.";
      continue;
    }
    const auto& tool_call = tool_call_raw.GetDict();

    // Most APIs that have partial chunk responses seem to initially always have
    // the tool name and id, and only chunk the arguments json. So whilst id and
    // name are required for the completed event, we can't rely on them being
    // present for parsing.

    const base::Value::Dict* function = tool_call.FindDict("function");
    if (!function) {
      DLOG(ERROR) << "No function info found in tool call.";
      continue;
    }

    const std::string* id = tool_call.FindString("id");
    const std::string* name = function->FindString("name");

    mojom::ToolUseEventPtr tool_use_event = mojom::ToolUseEvent::New(
        name ? *name : "", id ? *id : "", "", std::nullopt, nullptr);

    const std::string* arguments_raw = function->FindString("arguments");
    if (arguments_raw) {
      tool_use_event->arguments_json = *arguments_raw;
    }

    tool_use_events.push_back(std::move(tool_use_event));
  }

  return tool_use_events;
}

std::optional<base::Value::List> ToolApiDefinitionsFromTools(
    const std::vector<base::WeakPtr<Tool>>& tools) {
  if (tools.empty()) {
    return std::nullopt;
  }
  base::Value::List tools_list;
  for (const base::WeakPtr<Tool> tool : tools) {
    if (!tool) {
      DLOG(ERROR) << "Tool is null, skipping tool.";
      continue;
    }
    // Every tool needs a name otherwise it isn't useable
    if (tool->Name().empty()) {
      DLOG(ERROR) << "Tool name is empty, skipping tool.";
      continue;
    }

    base::Value::Dict tool_dict;

    bool type_is_funcion = tool->Type().empty() || tool->Type() == "function";
    tool_dict.Set("type", type_is_funcion ? "function" : tool->Type());

    if (type_is_funcion) {
      base::Value::Dict function_dict;
      function_dict.Set("name", tool->Name());

      if (!tool->Description().empty()) {
        function_dict.Set("description", tool->Description());
      }
      auto input_schema = tool->InputProperties();
      if (input_schema) {
        // input_schema contains the properties dict from the tool.
        // Wrap it in a proper JSON Schema object format.
        base::Value::Dict parameters;
        parameters.Set("type", "object");
        parameters.Set("properties", std::move(input_schema.value()));

        // We don't have any validation on parameters and required objects
        // as enforcing to JSON Schema is done by the remote and is non
        // fatal for the client.
        if (tool->RequiredProperties().has_value() &&
            !tool->RequiredProperties()->empty()) {
          base::Value::List required_properties;
          const auto properties = tool->RequiredProperties().value();
          for (const auto& property : properties) {
            required_properties.Append(property);
          }

          parameters.Set("required", std::move(required_properties));
        }

        function_dict.Set("parameters", std::move(parameters));
      }
      tool_dict.Set("function", std::move(function_dict));
    } else {
      // For non-known types (anything not "function"), we send name, type
      // and any "extra_param". The use case for this is remote-defined
      // tools that have different parameters to creat the tool description,
      // e.g. for screen size or user's locale.
      tool_dict.Set("name", tool->Name());
      if (tool->ExtraParams().has_value()) {
        tool_dict.Merge(tool->ExtraParams().value());
      }
    }
    tools_list.Append(std::move(tool_dict));
  }
  return tools_list;
}

std::optional<std::pair<mojom::ConversationEntryEventPtr, std::optional<std::string>>>
ParseResearchEvent(base::Value::Dict& response_event,
                   ModelService* model_service) {
  const std::string* type = response_event.FindString("type");
  if (!type) {
    return std::nullopt;
  }

  mojom::ConversationEntryEventPtr event;
  const std::string* model = response_event.FindString("model");

  if (*type == "research") {
    // Handle deep research events from the backend
    DVLOG(1) << "Processing research event";
    const base::Value::Dict* event_dict = response_event.FindDict("research");

    if (!event_dict) {
      DVLOG(1) << "Missing research field in research event";
      return std::nullopt;
    }

    const std::string* event_type = event_dict->FindString("event");
    if (!event_type) {
      DVLOG(1) << "Missing event.event field";
      return std::nullopt;
    }

    DVLOG(1) << "Research event type: " << *event_type;

    // Map event types to browser event types
    if (*event_type == "analyzing") {
      // Show searching indicator
      event = mojom::ConversationEntryEvent::NewSearchStatusEvent(
          mojom::SearchStatusEvent::New());
    } else if (*event_type == "thinking") {
      // Extract thinking event with sources
      const std::string* query = event_dict->FindString("query");
      const int urls_analyzed = event_dict->FindInt("urls_analyzed").value_or(0);
      const base::Value::List* urls_selected = event_dict->FindList("urls_selected");
      const base::Value::List* urls_info = event_dict->FindList("urls_info");

      auto thinking_event = mojom::ThinkingEvent::New();
      thinking_event->query = query ? *query : "";
      thinking_event->urls_analyzed = urls_analyzed;

      // Extract urls_selected
      if (urls_selected) {
        for (const auto& url : *urls_selected) {
          if (url.is_string()) {
            thinking_event->urls_selected.push_back(url.GetString());
          }
        }
      }

      // Extract urls_info with favicons
      if (urls_info) {
        for (const auto& url_info_value : *urls_info) {
          if (!url_info_value.is_dict()) {
            continue;
          }
          const base::Value::Dict& url_info_dict = url_info_value.GetDict();
          const std::string* url = url_info_dict.FindString("url");
          const std::string* favicon = url_info_dict.FindString("favicon");

          if (url) {
            auto url_info = mojom::UrlInfo::New();
            url_info->url = *url;
            url_info->favicon = favicon ? *favicon : "";
            thinking_event->urls_info.push_back(std::move(url_info));
          }
        }
      }

      event = mojom::ConversationEntryEvent::NewThinkingEvent(
          std::move(thinking_event));
    } else if (*event_type == "queries") {
      // Extract search queries
      const base::Value::List* queries = event_dict->FindList("queries");
      if (queries) {
        auto search_queries_event = mojom::SearchQueriesEvent::New();
        for (auto& item : *queries) {
          if (item.is_string()) {
            search_queries_event->search_queries.push_back(item.GetString());
          }
        }
        if (!search_queries_event->search_queries.empty()) {
          event = mojom::ConversationEntryEvent::NewSearchQueriesEvent(
              std::move(search_queries_event));
        } else {
          return std::nullopt;
        }
      } else {
        return std::nullopt;
      }
    } else if (*event_type == "answer") {
      // Extract completion text
      const std::string* answer = event_dict->FindString("answer");
      if (answer && !answer->empty()) {
        event = mojom::ConversationEntryEvent::NewCompletionEvent(
            mojom::CompletionEvent::New(*answer));
      } else {
        return std::nullopt;
      }
    } else if (*event_type == "insights") {
      // Extract sources from insights data
      const base::Value::Dict* insights = event_dict->FindDict("insights");
      if (insights) {
        auto web_sources_event = mojom::WebSourcesEvent::New();

        // Iterate through insights dict to extract source URLs
        for (auto [url_str, value] : *insights) {
          GURL source_url(url_str);
          if (!source_url.is_valid()) {
            continue;
          }

          // Extract hostname as title
          std::string title = std::string(source_url.host());
          GURL favicon_url(
              "chrome-untrusted://resources/brave-icons/globe.svg");

          web_sources_event->sources.push_back(
              mojom::WebSource::New(title, source_url, favicon_url));
        }

        if (!web_sources_event->sources.empty()) {
          event = mojom::ConversationEntryEvent::NewSourcesEvent(
              std::move(web_sources_event));
        } else {
          return std::nullopt;
        }
      } else {
        return std::nullopt;
      }
    } else if (*event_type == "videos") {
      // Extract video results from videos event
      const base::Value::List* videos = event_dict->FindList("videos");
      if (videos) {
        auto video_results_event = mojom::VideoResultsEvent::New();

        for (const auto& video_value : *videos) {
          if (!video_value.is_dict()) {
            continue;
          }

          const base::Value::Dict& video = video_value.GetDict();
          const std::string* title = video.FindString("title");
          const std::string* url = video.FindString("url");
          const std::string* thumbnail_url = video.FindString("thumbnail_url");

          // Require at least title and url
          if (!title || !url || title->empty() || url->empty()) {
            continue;
          }

          GURL video_url(*url);
          if (!video_url.is_valid()) {
            continue;
          }

          // Thumbnail is optional, use a default if not provided
          GURL thumbnail_gurl;
          if (thumbnail_url && !thumbnail_url->empty()) {
            thumbnail_gurl = GURL(*thumbnail_url);
            if (!thumbnail_gurl.is_valid()) {
              thumbnail_gurl =
                  GURL("chrome-untrusted://resources/brave-icons/video.svg");
            }
          } else {
            thumbnail_gurl =
                GURL("chrome-untrusted://resources/brave-icons/video.svg");
          }

          // Extract optional fields
          const std::string* age = video.FindString("age");
          const std::string* description = video.FindString("description");
          const std::string* duration = video.FindString("duration");
          const std::string* creator = video.FindString("creator");
          const std::string* publisher = video.FindString("publisher");

          video_results_event->videos.push_back(mojom::VideoResult::New(
              *title, video_url, thumbnail_gurl,
              age ? std::optional<std::string>(*age) : std::nullopt,
              description ? std::optional<std::string>(*description)
                          : std::nullopt,
              duration ? std::optional<std::string>(*duration) : std::nullopt,
              creator ? std::optional<std::string>(*creator) : std::nullopt,
              publisher ? std::optional<std::string>(*publisher)
                        : std::nullopt));
        }

        if (!video_results_event->videos.empty()) {
          event = mojom::ConversationEntryEvent::NewVideoResultsEvent(
              std::move(video_results_event));
        } else {
          return std::nullopt;
        }
      } else {
        return std::nullopt;
      }
    } else if (*event_type == "images") {
      // Extract image results from images event
      const base::Value::List* images = event_dict->FindList("images");
      if (images) {
        auto image_results_event = mojom::ImageResultsEvent::New();

        for (const auto& image_value : *images) {
          if (!image_value.is_dict()) {
            continue;
          }

          const base::Value::Dict& image = image_value.GetDict();
          const std::string* title = image.FindString("title");
          const std::string* url = image.FindString("url");
          const std::string* thumbnail_url = image.FindString("thumbnail_url");

          if (!title || !url || !thumbnail_url) {
            continue;
          }

          GURL image_gurl(*url);
          GURL thumbnail_gurl(*thumbnail_url);

          if (!image_gurl.is_valid() || !thumbnail_gurl.is_valid()) {
            continue;
          }

          const int width = image.FindInt("width").value_or(0);
          const int height = image.FindInt("height").value_or(0);

          image_results_event->images.push_back(mojom::ImageResult::New(
              *title, image_gurl, thumbnail_gurl,
              width > 0 ? std::optional<int>(width) : std::nullopt,
              height > 0 ? std::optional<int>(height) : std::nullopt));
        }

        if (!image_results_event->images.empty()) {
          event = mojom::ConversationEntryEvent::NewImageResultsEvent(
              std::move(image_results_event));
        } else {
          return std::nullopt;
        }
      } else {
        return std::nullopt;
      }
    } else if (*event_type == "news") {
      // Extract news results from news event
      const base::Value::List* news_list = event_dict->FindList("news");
      if (news_list) {
        auto news_results_event = mojom::NewsResultsEvent::New();

        for (const auto& news_value : *news_list) {
          if (!news_value.is_dict()) {
            continue;
          }

          const base::Value::Dict& news = news_value.GetDict();
          const std::string* title = news.FindString("title");
          const std::string* url = news.FindString("url");
          const std::string* thumbnail_url = news.FindString("thumbnail_url");
          const std::string* favicon = news.FindString("favicon");

          if (!title || !url || title->empty() || url->empty()) {
            continue;
          }

          GURL news_url(*url);
          if (!news_url.is_valid()) {
            continue;
          }

          GURL thumbnail_gurl(thumbnail_url && !thumbnail_url->empty()
                                  ? *thumbnail_url
                                  : "chrome-untrusted://resources/brave-icons/news.svg");
          GURL favicon_gurl(favicon && !favicon->empty()
                                ? *favicon
                                : "chrome-untrusted://resources/brave-icons/globe.svg");

          const std::string* age = news.FindString("age");
          const std::string* source = news.FindString("source");
          const bool is_breaking = news.FindBool("is_breaking").value_or(false);

          news_results_event->news.push_back(mojom::NewsResult::New(
              *title, news_url, thumbnail_gurl, favicon_gurl,
              age ? std::optional<std::string>(*age) : std::nullopt,
              source ? std::optional<std::string>(*source) : std::nullopt,
              is_breaking ? std::optional<bool>(is_breaking) : std::nullopt));
        }

        if (!news_results_event->news.empty()) {
          event = mojom::ConversationEntryEvent::NewNewsResultsEvent(
              std::move(news_results_event));
        } else {
          return std::nullopt;
        }
      } else {
        return std::nullopt;
      }
    } else if (*event_type == "discussions") {
      // Extract discussion results from discussions event
      const base::Value::List* discussions_list = event_dict->FindList("discussions");
      if (discussions_list) {
        auto discussion_results_event = mojom::DiscussionResultsEvent::New();

        for (const auto& discussion_value : *discussions_list) {
          if (!discussion_value.is_dict()) {
            continue;
          }

          const base::Value::Dict& discussion = discussion_value.GetDict();
          const std::string* title = discussion.FindString("title");
          const std::string* url = discussion.FindString("url");

          if (!title || !url || title->empty() || url->empty()) {
            continue;
          }

          GURL discussion_url(*url);
          if (!discussion_url.is_valid()) {
            continue;
          }

          const std::string* description = discussion.FindString("description");
          const std::string* favicon = discussion.FindString("favicon");
          const std::string* age = discussion.FindString("age");
          const std::string* forum_name = discussion.FindString("forum_name");
          const int num_answers = discussion.FindInt("num_answers").value_or(0);

          GURL favicon_gurl(favicon && !favicon->empty()
                                ? *favicon
                                : "chrome-untrusted://resources/brave-icons/globe.svg");

          discussion_results_event->discussions.push_back(
              mojom::DiscussionResult::New(
                  *title, discussion_url,
                  description ? std::optional<std::string>(*description)
                              : std::nullopt,
                  favicon_gurl,
                  age ? std::optional<std::string>(*age) : std::nullopt,
                  forum_name ? std::optional<std::string>(*forum_name)
                             : std::nullopt,
                  num_answers > 0 ? std::optional<int>(num_answers)
                                  : std::nullopt));
        }

        if (!discussion_results_event->discussions.empty()) {
          event = mojom::ConversationEntryEvent::NewDiscussionResultsEvent(
              std::move(discussion_results_event));
        } else {
          return std::nullopt;
        }
      } else {
        return std::nullopt;
      }
    } else if (*event_type == "blindspots") {
      // Extract blindspots (knowledge gaps)
      const base::Value::List* blindspots = event_dict->FindList("blindspots");
      if (blindspots && !blindspots->empty()) {
        auto blindspots_event = mojom::BlindspotsEvent::New();
        for (const auto& item : *blindspots) {
          if (item.is_string()) {
            blindspots_event->blindspots.push_back(item.GetString());
          }
        }
        if (!blindspots_event->blindspots.empty()) {
          event = mojom::ConversationEntryEvent::NewBlindspotsEvent(
              std::move(blindspots_event));
        } else {
          return std::nullopt;
        }
      } else {
        return std::nullopt;
      }
    } else if (*event_type == "progress") {
      // Extract progress update
      const int iteration = event_dict->FindInt("iteration").value_or(0);
      const double elapsed_seconds = event_dict->FindDouble("elapsed_seconds").value_or(0.0);
      const int urls_analyzed = event_dict->FindInt("urls_analyzed").value_or(0);
      const int queries_issued = event_dict->FindInt("queries_issued").value_or(0);

      auto progress_event = mojom::ProgressEvent::New();
      progress_event->iteration = iteration;
      progress_event->elapsed_seconds = elapsed_seconds;
      progress_event->urls_analyzed = urls_analyzed;
      progress_event->queries_issued = queries_issued;

      event = mojom::ConversationEntryEvent::NewProgressEvent(
          std::move(progress_event));
    } else if (*event_type == "ping") {
      // Ignore ping events
      return std::nullopt;
    } else {
      // Unknown event type, ignore
      DVLOG(1) << "Unknown research event type: " << *event_type;
      return std::nullopt;
    }
  } else if (*type == "research_start") {
    // Handle research start event (shows searching indicator)
    event = mojom::ConversationEntryEvent::NewSearchStatusEvent(
        mojom::SearchStatusEvent::New());
  } else if (*type == "completion") {
    // Handle completion events (text deltas from deep research final answer)
    const std::string* completion = response_event.FindString("completion");
    if (completion && !completion->empty()) {
      event = mojom::ConversationEntryEvent::NewCompletionEvent(
          mojom::CompletionEvent::New(*completion));
    } else {
      return std::nullopt;
    }
  } else if (*type == "conversationTitle") {
    // Handle conversation title events
    const std::string* title = response_event.FindString("title");
    if (title && !title->empty()) {
      event = mojom::ConversationEntryEvent::NewConversationTitleEvent(
          mojom::ConversationTitleEvent::New(*title));
    } else {
      return std::nullopt;
    }
  } else {
    // Not a research event
    return std::nullopt;
  }

  // Return the event and model key
  if (model && model_service) {
    return std::make_pair(std::move(event),
                          model_service->GetLeoModelKeyByName(*model));
  }
  return std::make_pair(std::move(event), std::nullopt);
}

}  // namespace ai_chat
