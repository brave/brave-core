#include "brave/ios/browser/api/bookmarks/exporter/bookmarks_encoder.h"
#include "brave/ios/browser/api/bookmarks/exporter/exported_bookmark_entry.h"
#include "components/bookmarks/browser/bookmark_codec.h"

#include <stddef.h>
#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/guid.h"
#include "base/json/json_string_value_serializer.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

using base::Time;

using bookmarks::BookmarkCodec;

namespace ios {
// Current version of the file.
static const int kCurrentVersion = 1;

class BookmarkEncoder {
public:
  BookmarkEncoder();
  ~BookmarkEncoder();
  
  std::unique_ptr<base::Value> Encode(
      const ExportedBookmarkEntry* bookmark_bar_node,
      const ExportedBookmarkEntry* other_folder_node,
      const ExportedBookmarkEntry* mobile_folder_node,
      const std::string& sync_metadata_str);
  
private:
  std::unique_ptr<base::Value> EncodeNode(const ExportedBookmarkEntry* node);
  
  // Updates the check-sum with the given string.
  void UpdateChecksum(const std::string& str);
  void UpdateChecksum(const base::string16& str);

  // Updates the check-sum with the given contents of URL/folder bookmark node.
  // NOTE: These functions take in individual properties of a bookmark node
  // instead of taking in a BookmarkNode for efficiency so that we don't convert
  // various data-types to UTF16 strings multiple times - once for serializing
  // and once for computing the check-sum.
  // The url parameter should be a valid UTF8 string.
  void UpdateChecksumWithUrlNode(const std::string& id,
                                 const base::string16& title,
                                 const std::string& url);
  void UpdateChecksumWithFolderNode(const std::string& id,
                                    const base::string16& title);

  // Initializes/Finalizes the checksum.
  void InitializeChecksum();
  void FinalizeChecksum();
  
  // MD5 context used to compute MD5 hash of all bookmark data.
  base::MD5Context md5_context_;
  
  // Checksums.
  std::string computed_checksum_;
  std::string stored_checksum_;
};

BookmarkEncoder::BookmarkEncoder() = default;

BookmarkEncoder::~BookmarkEncoder() = default;

void BookmarkEncoder::UpdateChecksum(const std::string& str) {
  base::MD5Update(&md5_context_, str);
}

void BookmarkEncoder::UpdateChecksum(const base::string16& str) {
  base::MD5Update(&md5_context_,
                  base::StringPiece(
                      reinterpret_cast<const char*>(str.data()),
                      str.length() * sizeof(str[0])));
}

void BookmarkEncoder::UpdateChecksumWithUrlNode(const std::string& id,
                               const base::string16& title,
                               const std::string& url) {
  DCHECK(base::IsStringUTF8(url));
  UpdateChecksum(id);
  UpdateChecksum(title);
  UpdateChecksum(bookmarks::BookmarkCodec::kTypeURL);
  UpdateChecksum(url);
}

void BookmarkEncoder::UpdateChecksumWithFolderNode(const std::string& id,
                                                 const base::string16& title) {
  UpdateChecksum(id);
  UpdateChecksum(title);
  UpdateChecksum(bookmarks::BookmarkCodec::kTypeFolder);
}

void BookmarkEncoder::InitializeChecksum() {
  base::MD5Init(&md5_context_);
}

void BookmarkEncoder::FinalizeChecksum() {
  base::MD5Digest digest;
  base::MD5Final(&digest, &md5_context_);
  computed_checksum_ = base::MD5DigestToBase16(digest);
}

std::unique_ptr<base::Value> BookmarkEncoder::EncodeNode(const ExportedBookmarkEntry* node) {
  std::unique_ptr<base::DictionaryValue> value(new base::DictionaryValue());
  std::string id = base::NumberToString(node->id());
  value->SetString(bookmarks::BookmarkCodec::kIdKey, id);
  const base::string16& title = node->GetTitle();
  value->SetString(bookmarks::BookmarkCodec::kNameKey, title);
  const std::string& guid = node->guid();
  value->SetString(bookmarks::BookmarkCodec::kGuidKey, guid);
  value->SetString(bookmarks::BookmarkCodec::kDateAddedKey,
                   base::NumberToString(node->date_added().ToInternalValue()));
  if (node->is_url()) {
    value->SetString(bookmarks::BookmarkCodec::kTypeKey, bookmarks::BookmarkCodec::kTypeURL);
    std::string url = node->url().possibly_invalid_spec();
    value->SetString(bookmarks::BookmarkCodec::kURLKey, url);
    UpdateChecksumWithUrlNode(id, title, url);
  } else {
    value->SetString(bookmarks::BookmarkCodec::kTypeKey, bookmarks::BookmarkCodec::kTypeFolder);
    value->SetString(
        bookmarks::BookmarkCodec::kDateModifiedKey,
        base::NumberToString(node->date_folder_modified().ToInternalValue()));
    UpdateChecksumWithFolderNode(id, title);

    auto child_values = std::make_unique<base::ListValue>();
    for (const auto& child : node->children())
      child_values->Append(EncodeNode(child.get()));
    value->Set(bookmarks::BookmarkCodec::kChildrenKey, std::move(child_values));
  }
  return std::move(value);
}

std::unique_ptr<base::Value> BookmarkEncoder::Encode(const ExportedBookmarkEntry* bookmark_bar_node,
                                                     const ExportedBookmarkEntry* other_folder_node,
                                                     const ExportedBookmarkEntry* mobile_folder_node,
                                                     const std::string& sync_metadata_str) {
  InitializeChecksum();
  auto roots = std::make_unique<base::DictionaryValue>();
  roots->Set(bookmarks::BookmarkCodec::kRootFolderNameKey, EncodeNode(bookmark_bar_node));
  roots->Set(bookmarks::BookmarkCodec::kOtherBookmarkFolderNameKey, EncodeNode(other_folder_node));
  roots->Set(bookmarks::BookmarkCodec::kMobileBookmarkFolderNameKey, EncodeNode(mobile_folder_node));
  
  auto main = std::make_unique<base::DictionaryValue>();
  main->SetInteger(bookmarks::BookmarkCodec::kVersionKey, kCurrentVersion);
  FinalizeChecksum();
  
  // We are going to store the computed checksum. So set stored checksum to be
  // the same as computed checksum.
  stored_checksum_ = computed_checksum_;
  main->SetString(bookmarks::BookmarkCodec::kChecksumKey, computed_checksum_);
  main->Set(bookmarks::BookmarkCodec::kRootsKey, std::move(roots));
  if (!sync_metadata_str.empty()) {
    std::string sync_metadata_str_base64;
    base::Base64Encode(sync_metadata_str, &sync_metadata_str_base64);
    main->SetKey(bookmarks::BookmarkCodec::kSyncMetadata,
                 base::Value(std::move(sync_metadata_str_base64)));
  }
  return std::move(main);
}
} // namespace ios

namespace ios {
namespace bookmarks_encoder {
std::unique_ptr<base::Value>
    EncodeBookmarks(std::unique_ptr<ExportedRootBookmarkEntry> root_node) {
  
  auto encoder = std::make_unique<ios::BookmarkEncoder>();
  return encoder->Encode(root_node->bookmarks_bar_node(),
                         root_node->other_bookmarks_node(),
                         root_node->mobile_bookmarks_node(),
                         /*sync_metadata_str*/ std::string());
}
} // namespace bookmarks_encoder
} // namespace ios
