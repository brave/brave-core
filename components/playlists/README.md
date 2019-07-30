# Playlists API

## Calling the API from C++

You will need some or all of the following headers:

``` c++
#include "brave/browser/playlists/playlists_service_factory.h"
#include "brave/components/playlists/browser/playlists_controller.h"
#include "brave/components/playlists/browser/playlists_controller_observer.h"
#include "brave/components/playlists/browser/playlists_media_file_controller.h"
#include "brave/components/playlists/browser/playlists_service.h"
#include "brave/components/playlists/browser/playlists_types.h"
```

The Playlists service is a `BrowserContextKeyedService`, so you will access it through the `PlaylistsServiceFactory`.

``` c++
PlaylistsService* playlists_service = PlaylistsServiceFactory::GetInstance()->
  GetForProfile(SOME_PROFILE);
```

There are only two interesting things in the `PlaylistsService`: an `Init()` method and a `controller()` method. You need to call the `Init()` method exactly once so it can set up the internal database it uses to track local playlists. It takes no parameters and returns immediately, but the Playlist API is not immediately ready. You can use the controller to check whether initialization is complete.

Which brings us to the other interesting method, `controller()`:

``` c++
PlaylistsController* playlists_controller = playlists_service->controller();
```

You must call `controller()->initialized()` and verify that it returns `true` before calling any of the other interesting methods on the `PlaylistController`. This is enforced with `DCHECK`, but since initialization happens asynchronously, race conditions are always possible. So always verify.

Besides `initialize()`, there are a number of interesting methods on the `PlaylistController`.

### `CreatePlaylist`

`CreatePlaylist` creates a new playlist, which creates a new record in the local database, creates local directories in the file system to hold all the files for this playlist, then starts downloading assets (e.g. thumbnail, video and audio files). The method itself returns almost immediately; the rest of the work, including all network access, happens asynchronously.

The `CreatePlaylist` method takes a `CreatePlaylistParams` struct, which has a bunch of members, including `media_files`, which is a vector of `MediaFileInfo`, which is its own struct. See `playlist_types.h` for all the details.  All parameters are required.

You can follow the progress of the asynchronous tasks by setting up a `PlaylistsControllerObserver` (see below). You almost certainly want to do this, as it is the only way to get the ID of a newly created playlist (required for other API calls) or to know whether the playlist creation succeeded at all.

### `GetPlaylist`

`GetPlaylist` returns information about a specific playlist by ID. The method takes a playlist ID and a callback, which is called with a JSON dictionary (`base::Value`) containing information about the playlist, including ID, name, path to the downloaded thumbnail image, path to the download media files, and a copy of the original parameters passed to `CreatePlaylist` (which includes the origin remote URLs).

You should use the constants in `playlists_constants.h` to access specific values within the JSON dictionary.

### `GetAllPlaylists`

`GetAllPlaylists` returns information about all local playlists. The method takes a callback, which is called with a JSON array of dictionaries. Each dictionary in the array is identical to what you would get from the `GetPlaylist` method for that playlist.

You should use the constants in `playlists_constants.h` to access specific values within the JSON dictionary.

### `DeletePlaylist`

`DeletePlaylist` deletes a specific playlist by ID, including all database records and all local files and directories associated with the playlist. If the playlist was recently created and is still in the process of downloading assets or doing local file work, those operations are cancelled.

This is a "physical delete," not a "logical delete." Database records are removed, not simply marked as inactive. Files and directories on the filesystem are deleted. As such, there is no way to undo this operation.

The method takes an ID and returns almost immediately; all actual file and database work occurs asynchronously.

You can follow the progress of the asynchronous tasks by setting up a `PlaylistsControllerObserver` (see below).

### `DeleteAllPlaylists`

`DeleteAllPlaylists` deletes all playlists. It takes a callback which will be called with a boolean that will be `true` if all deletion was successful.

### `RecoverPlaylist`

TODO

### `RequestDownload`

`RequestDownload` takes a URL and, based on the origin, initiates a vendor-specific process to determine if it can create a playlist from that URL. If so, it calls `CreatePlaylist` with the relevant media files and other metadata.

The vendor-specific part relies on a third-party library, which is bundled with Brave but is not written in C++. As such, this function doesn't do any real work; it only broadcasts an event. In effect, it yells into the void, "oh hey, the user requested a download, and it would be great if somebody could look into that." The Brave Playlist extension listens for these events and acts as a liaison between the third-party library and the rest of the Playlist API.

### Observing Playlist API Notifications

Most real work by the Playlist API functions happens asynchronously, so you will almost certainly want to set up an observer to keep track of what is going on.

Inherit a class from `PlaylistsControllerObserver` and override all of the following methods:

* `OnPlaylistsInitialized` is called when the Playlist API completes its initialization (after calling the `Init()` method on the `PlaylistService`). This callback takes a boolean that indicates whether initialization succeeded.
* `OnPlaylistsDownloadRequested` is called whenever someone requests a download (via the `RequestDownload` method). You probably don't need to do anything here, unless you are responsible for downloading and parsing a remote URL to look for potential playlists, or if you are interfacing with a library that does that.
* `OnPlaylistsChanged` is the workhorse. It is called whenever something interesting happens. It takes a `PlaylistsChangeParams` struct, which has a `playlist_id` and `change_type` (defined in `playlist_types.h`). The change type tells you what just happened.
  * `CHANGE_TYPE_ADDED` means a new playlist was added but it's not ready yet
  * `CHANGE_TYPE_THUMBNAIL_READY` means a thumbnail was downloaded, has been saved in the local filesystem and is ready to display.
  * `CHANGE_TYPE_PLAY_READY` means all the video/audio assets of a playlist have been downloaded and the playlist is ready to play.
  * `CHANGE_TYPE_DELETED` means a playlist was deleted, probably because you called `DeletePlaylist`.
  * `CHANGE_TYPE_ALL_DELETED` means all playlists were deleted, probably because you called `DeleteAllPlaylists`

There are also several error conditions you can watch for, including (most commonly) failure to download thumbnails or video assets. See `playlist_types.h` for details.

## Calling the API from TypeScript or JavaScript

`chrome.bravePlaylists`

TODO
