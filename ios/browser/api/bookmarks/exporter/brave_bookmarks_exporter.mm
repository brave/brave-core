#include "brave/ios/browser/api/bookmarks/exporter/brave_bookmarks_exporter.h"

#include <vector>
#include <functional>
#include "brave/ios/browser/api/bookmarks/exporter/bookmark_html_writer.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/files/file_path.h"
#include "base/base_paths.h"
#include "base/path_service.h"
#include "ios/chrome/browser/application_context.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state_manager.h"

class BraveBookmarksExportObserver: public BookmarksExportObserver {
public:
  BraveBookmarksExportObserver(std::function<void(BraveBookmarksExporterState)> on_export_finished);
  void OnExportFinished(Result result) override;
    
private:
  std::function<void(BraveBookmarksExporterState)> _on_export_finished;
};

BraveBookmarksExportObserver::BraveBookmarksExportObserver(std::function<void(BraveBookmarksExporterState)> on_export_finished):
    _on_export_finished(on_export_finished) {
}

void BraveBookmarksExportObserver::OnExportFinished(Result result) {
  switch(result) {
    case Result::kSuccess:
      return _on_export_finished(BraveBookmarksExporterStateCompleted);
    case Result::kCouldNotCreateFile:
      return _on_export_finished(BraveBookmarksExporterStateErrorCreatingFile);
    case Result::kCouldNotWriteHeader:
      return _on_export_finished(BraveBookmarksExporterStateErrorWritingHeader);
    case Result::kCouldNotWriteNodes:
      return _on_export_finished(BraveBookmarksExporterStateErrorWritingNodes);
    default:
      NOTREACHED();
  }
  delete this;
}

@interface BraveBookmarksExporter()
@end

@implementation BraveBookmarksExporter

- (instancetype)init {
  if ((self = [super init])) {
      
  }
  return self;
}

- (void)exportToFile:(NSString *)filePath
       withListener:(void(^)(BraveBookmarksExporterState))listener {
    
  base::FilePath destination_file_path =
      base::FilePath::FromUTF8Unsafe([filePath UTF8String]);
    
  listener(BraveBookmarksExporterStateStarted);
    
  ios::ChromeBrowserStateManager* browserStateManager =
      GetApplicationContext()->GetChromeBrowserStateManager();
  ChromeBrowserState* chromeBrowserState =
      browserStateManager->GetLastUsedBrowserState();
    
  bookmark_html_writer::WriteBookmarks(chromeBrowserState,
      destination_file_path,
      new BraveBookmarksExportObserver(listener)
  );
}
@end
