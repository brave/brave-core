(function() {
  const script = document.createElement('script');
  // Modify the listener by ignoring visibilitychange
  // so when the app goes in background the video
  // is not paused.
  script.textContent = `
  if (document._addEventListener === undefined) {
    document._addEventListener = document.addEventListener;
    document.addEventListener = function(a,b,c) {
      if(a != 'visibilitychange') {
        document._addEventListener(a,b,c);
      }
    };
  }
  `;
  
  document.head.appendChild(script);
  script.remove();
}());