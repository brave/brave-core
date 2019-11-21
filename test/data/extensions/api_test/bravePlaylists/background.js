/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

function testHasAccess() {
  chrome.test.runTests([
    function hasAccess() {
      if (chrome.bravePlaylists) {
        chrome.test.succeed();
      } else {
        chrome.test.fail();
      }
    }
  ]);
}

function testInitialize() {
  chrome.test.runTests([
    function initialize() {
      if (chrome.bravePlaylists.isInitialized(function(init) {
        if (init) {
          // API should not be initialized yet.
          chrome.test.fail();
        }
        chrome.bravePlaylists.initialize(function() {
          // Calling initialize() API function should call the callback
          // function once initialization is complete.
          chrome.test.succeed();
        });
      }));
    },

    function onInitializedEvent() {
      // Calling initialize() API should send the onInitialized event to
      // all listeners.
      chrome.bravePlaylists.onInitialized.addListener(function(init) {
        chrome.test.succeed();
      });
      if (chrome.bravePlaylists.isInitialized(function(init) {
        if (init) {
          // API should not be initialized yet.
          chrome.test.fail();
        }
        chrome.bravePlaylists.initialize(function() {});
      }));
    }
  ]);
}

function testCreatePlaylistNoCrash() {
  chrome.test.runTests([
    function createPlaylistNoCrash() {
      chrome.bravePlaylists.onInitialized.addListener(function(init) {
        // Calling createPlaylist() API should not crash, even if all
        // parameters are empty.
        if (!init) { chrome.test.fail(); }
        chrome.bravePlaylists.createPlaylist({
          thumbnailUrl: '',
          playlistName: '',
          videoMediaFiles: [],
          audioMediaFiles: []
        });
        chrome.test.succeed();
      });
      chrome.bravePlaylists.initialize(function() {});
    }
  ]);
}

function createPlaylistAndWait(thumbnail_url, video_url, audio_url, expected_change_types, callback) {
  in_progress = true;
  chrome.bravePlaylists.onPlaylistsChanged.addListener(function(changeType, id) {
    if (changeType != expected_change_types[0]) {
      console.log('expected changeType=' + expected_change_types[0] + ' but got changeType=' + changeType);
      callback(false);
    }
    expected_change_types.shift();
    if (expected_change_types.length == 0) {
      // We got all the events in the right order, so we're done.
      callback(true);
    }
  });
  chrome.bravePlaylists.onInitialized.addListener(function(init) {
    if (!init) { chrome.test.fail(); }
    if (video_url) {
      video_media_files = [{url:video_url, title:'test', thumb:''}];
    } else {
      video_media_files = [];
    }
    if (audio_url) {
      audio_media_files = [{url:audio_url, title:'test', thumb:''}];
    } else {
      audio_media_files = [];
    }
    chrome.bravePlaylists.createPlaylist({
      thumbnailUrl: thumbnail_url,
      playlistName: 'test',
      videoMediaFiles: video_media_files,
      audioMediaFiles: audio_media_files
    });
  });
  chrome.bravePlaylists.initialize(function() {});
}

function testCreatePlaylist(thumbnail_url, video_url, audio_url, expected_change_types) {
  chrome.test.runTests([
    function createPlaylist() {
      createPlaylistAndWait(thumbnail_url, video_url, audio_url, expected_change_types, function(success) {
        if (success) {
          chrome.test.succeed();
        } else {
          chrome.test.fail();
        }
      });
    }
  ]);
}
