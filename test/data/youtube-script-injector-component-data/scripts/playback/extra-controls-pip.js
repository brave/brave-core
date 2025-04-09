(function() {
  const script = document.createElement('script');
  script.textContent = `
  function enterPictureInPicture() {
    brave?.nativePipMode();
  }
  `;

  document.head.appendChild(script);
  script.remove();
}());

const observer = new MutationObserver((_mutationsList) => {
  addPipButton();
});
  
function addPipButton() {
  const fullscreenButton = document.querySelector("button.fullscreen-icon");
  if (!fullscreenButton) {
    return;
  }

  const fullscreenButtonParent = fullscreenButton.closest("div");
  if (!fullscreenButtonParent) {
    return;
  }

  if (fullscreenButtonParent.querySelector("button.pip-icon")) {
    return;
  }

  fullscreenButtonParent.style.display = "flex";
  const pipButtonHTML = `
  <button class="icon-button pip-icon" aria-label="Enter picture-in-picture mode" onclick="enterPictureInPicture()">
  <c3-icon style="">
    <span class="yt-icon-shape yt-spec-icon-shape">
      <div style="width: 100%; height: 100%; display: block; fill: currentcolor;">
        <svg xmlns="http://www.w3.org/2000/svg" enable-background="new 0 0 24 24" height="24" viewBox="0 0 24 24" width="24" focusable="false" aria-hidden="true" style="pointer-events: none; display: inherit; width: 100%; height: 100%;">
          <path d="M6,6h12v12H6V6z M15,9h-4v4h4V9z"></path>
        </svg>
      </div>
    </span>
  </c3-icon>
  </button>
  `;
  fullscreenButtonParent.insertAdjacentHTML("afterbegin", pipButtonHTML);
  return true;
}

window.addEventListener("load", () => {
  addPipButton();
  observer.observe(document.body, { childList: true, subtree: true });
});
