let urls = [...document.querySelectorAll("a")]
    .map(d => { 
        if (d.href.startsWith("http")) {
            return d.href;
        }
    });
uniq_urls = [...new Set(urls)];
uniq_urls.sort();
JSON.stringify(uniq_urls, null, 2)