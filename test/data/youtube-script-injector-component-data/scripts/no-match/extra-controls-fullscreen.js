function triggerFullscreen() {
  // Always play video before entering fullscreen mode.
document.querySelector("video.html5-main-video")?.play();
// Check if the video is not in fullscreen mode already.
if (!document.fullscreenElement) {
  // Create a MutationObserver to watch for changes in the DOM.
  const observer = new MutationObserver((_mutationsList, observer) => {
    var fullscreenBtn = document.querySelector("button.fullscreen-icon");
    if (fullscreenBtn) {
      observer.disconnect()
      fullscreenBtn.click();
    }
  });

  var fullscreenBtn = document.querySelector("button.fullscreen-icon");
  // Check if fullscreen button is available.
  if (fullscreenBtn) {
    fullscreenBtn.click();
  } else {
    // When fullscreen button is not available
    // clicking the movie player resume the UI.
    var moviePlayer = document.getElementById("movie_player");
    if (moviePlayer) {
      // Start observing the DOM.
      observer.observe(document.body, { childList: true, subtree: true });
      // Make sure the player is in focus or responsive.
      moviePlayer.click();
    }
  }
}
}

if (document.readyState === "loading") {
  // Loading hasn't finished yet.
  document.addEventListener("DOMContentLoaded", triggerFullscreen);
} else {
  // `DOMContentLoaded` has already fired.
  triggerFullscreen();
}