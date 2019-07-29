var tag = document.getElementsByTagName("span");
  for(var i = 0, l = tag.length; i < l; i++) {
      var el = tag[i];
      el.innerHTML = el.innerHTML.replace(/ Chrome/gi, ' Brave');
}
