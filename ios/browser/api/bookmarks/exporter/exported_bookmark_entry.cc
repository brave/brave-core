#include "brave/ios/browser/api/bookmarks/exporter/exported_bookmark_entry.h"

#include <string>
#include "base/guid.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

// Whitespace characters to strip from bookmark titles.
const base::char16 kInvalidChars[] = {
  '\n', '\r', '\t',
  0x2028,  // Line separator
  0x2029,  // Paragraph separator
  0
};

}  // namespace

ExportedRootBookmarkEntry::ExportedRootBookmarkEntry(
                          std::unique_ptr<ExportedBookmarkEntry> root_node,
                          ExportedBookmarkEntry* bookmarks_bar_node,
                          ExportedBookmarkEntry* other_bookmarks_node,
                          ExportedBookmarkEntry* mobile_bookmarks_node)
    : root_node_(std::move(root_node)),
      bookmarks_bar_node_(bookmarks_bar_node),
      other_bookmarks_node_(other_bookmarks_node),
      mobile_bookmarks_node_(mobile_bookmarks_node) {}

ExportedRootBookmarkEntry::~ExportedRootBookmarkEntry() {}


ExportedBookmarkEntry::ExportedBookmarkEntry(int64_t id,
                                             const std::string& guid,
                                             const GURL& url)
    :  id_(id),
       guid_(guid),
       url_(url),
       type_(url.is_empty() ?
             bookmarks::BookmarkNode::FOLDER : bookmarks::BookmarkNode::URL),
       date_added_(base::Time::Now()) {
         
  DCHECK((type_ == bookmarks::BookmarkNode::Type::URL) != url.is_empty());
  DCHECK(base::IsValidGUIDOutputString(guid));
}

ExportedBookmarkEntry::ExportedBookmarkEntry(int64_t id,
                                             bookmarks::BookmarkNode::Type type,
                                             const std::string& guid,
                                             const base::string16& title)
    : id_(id),
      guid_(base::GenerateGUID()),
      url_(),
      type_(type),
      date_added_(base::Time::Now()) {

  DCHECK((type == bookmarks::BookmarkNode::Type::URL) != url_.is_empty());
  DCHECK(base::IsValidGUIDOutputString(guid));
  DCHECK(type != bookmarks::BookmarkNode::URL);
  SetTitle(title);
}

ExportedBookmarkEntry::~ExportedBookmarkEntry() {}

void ExportedBookmarkEntry::SetTitle(const base::string16& title) {
  // Replace newlines and other problematic whitespace characters in
  // folder/bookmark names with spaces.
  base::string16 trimmed_title;
  base::ReplaceChars(title, kInvalidChars, base::ASCIIToUTF16(" "),
                     &trimmed_title);
  title_ = trimmed_title;
}

ExportedBookmarkEntry* ExportedBookmarkEntry::Add(std::unique_ptr<ExportedBookmarkEntry> node) {
  DCHECK(node);
  children_.push_back(std::move(node));
  return children_.back().get();
}

// static
std::unique_ptr<ExportedRootBookmarkEntry> ExportedBookmarkEntry::get_root_node() {
  auto root_node_ = std::make_unique<ExportedBookmarkEntry>(0,
                                                            bookmarks::BookmarkNode::kRootNodeGuid,
                                                            GURL());
  
  ExportedBookmarkEntry* bb_node_ = static_cast<ExportedBookmarkEntry*>(
      root_node_->Add(ExportedBookmarkEntry::CreateBookmarkBar(1)));
  
  ExportedBookmarkEntry* other_folder_node_ = static_cast<ExportedBookmarkEntry*>(
      root_node_->Add(ExportedBookmarkEntry::CreateOtherBookmarks(2)));

  ExportedBookmarkEntry* mobile_folder_node_ = static_cast<ExportedBookmarkEntry*>(
      root_node_->Add(ExportedBookmarkEntry::CreateMobileBookmarks(3)));
  
  return std::make_unique<ExportedRootBookmarkEntry>(std::move(root_node_),
                                                     bb_node_,
                                                     other_folder_node_,
                                                     mobile_folder_node_);
}

// static
std::unique_ptr<ExportedBookmarkEntry>
ExportedBookmarkEntry::CreateBookmarkBar(int64_t id) {
  // base::WrapUnique() used because the constructor is private.
  return base::WrapUnique(new ExportedBookmarkEntry(
      id, bookmarks::BookmarkNode::Type::BOOKMARK_BAR, bookmarks::BookmarkNode::kBookmarkBarNodeGuid,
      l10n_util::GetStringUTF16(IDS_BOOKMARK_BAR_FOLDER_NAME)));
}

// static
std::unique_ptr<ExportedBookmarkEntry>
ExportedBookmarkEntry::CreateOtherBookmarks(int64_t id) {
  // base::WrapUnique() used because the constructor is private.
  return base::WrapUnique(new ExportedBookmarkEntry(
      id, bookmarks::BookmarkNode::Type::OTHER_NODE, bookmarks::BookmarkNode::kOtherBookmarksNodeGuid,
      l10n_util::GetStringUTF16(IDS_BOOKMARK_BAR_OTHER_FOLDER_NAME)));
}

// static
std::unique_ptr<ExportedBookmarkEntry>
ExportedBookmarkEntry::CreateMobileBookmarks(int64_t id) {
  // base::WrapUnique() used because the constructor is private.
  return base::WrapUnique(new ExportedBookmarkEntry(
      id, bookmarks::BookmarkNode::Type::MOBILE, bookmarks::BookmarkNode::kMobileBookmarksNodeGuid,
      l10n_util::GetStringUTF16(IDS_BOOKMARK_BAR_MOBILE_FOLDER_NAME)));
}
