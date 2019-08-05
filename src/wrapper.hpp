#ifndef ADBLOCK_RUST_FFI_SRC_WRAPPER_HPP_
#define ADBLOCK_RUST_FFI_SRC_WRAPPER_HPP_
#include <memory>
#include <string>
#include <vector>

extern "C" {
#include "lib.h"
}

namespace adblock {

class FilterList {
 public:
  FilterList(const std::string& uuid,
             const std::string& url,
             const std::string& title,
             const std::vector<std::string>& langs,
             const std::string& support_url,
             const std::string& component_id,
             const std::string& base64_public_key);
  FilterList(const FilterList& other);
  ~FilterList();

  static std::vector<FilterList>& GetDefaultLists();
  static std::vector<FilterList>& GetRegionalLists();

  const std::string uuid;
  const std::string url;
  const std::string title;
  const std::vector<std::string> langs;
  const std::string support_url;
  const std::string component_id;
  const std::string base64_public_key;

private:
  static std::vector<FilterList>& GetFilterLists(const std::string &category);
  static std::vector<FilterList> default_list;
  static std::vector<FilterList> regional_list;
};

class Engine {
 public:
  Engine();
  Engine(const std::string& rules);
  bool matches(const std::string& url, const std::string& host,
      const std::string& tab_host, bool is_third_party,
      const std::string& resource_type, bool* explicit_cancel,
      bool* saved_from_exception,
      std::string *redirect);
  bool deserialize(const char* data, size_t data_size);
  void addFilter(const std::string& filter);
  void addTag(const std::string& tag);
  void addResource(const std::string& key,
      const std::string& content_type,
      const std::string& data);
  void addResources(const std::string& resources);
  void removeTag(const std::string& tag);
  bool tagExists(const std::string& tag);
  const std::string classIdStylesheet(
      const std::vector<std::string>& classes,
      const std::vector<std::string>& ids,
      const std::vector<std::string>& exceptions
  );
  ~Engine();

 private:
  Engine(const Engine&) = delete;
  void operator=(const Engine&) = delete;
  C_Engine* raw;
};

}  // namespace adblock

#endif  // ADBLOCK_RUST_FFI_SRC_WRAPPER_HPP_
