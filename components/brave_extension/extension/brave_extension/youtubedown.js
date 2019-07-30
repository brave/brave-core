/* Copyright (c) 2019 Jamie Zawinski <jwz@jwz.org>

   Permission to use, copy, modify, distribute, and sell this software and its
   documentation for any purpose is hereby granted without fee, provided that
   the above copyright notice appear in all copies and that both that
   copyright notice and this permission notice appear in supporting
   documentation.  No representations are made about the suitability of this
   software for any purpose.  It is provided "as is" without express or
   implied warranty.

   A JavaScript program to download youtube videos from within your browser.

   See also the Perl version, https://www.jwz.org/hacks/#youtubedown

   To use this, bookmark this URL:

      javascript:(function(){var%20s=document.createElement('script');s.onload=function(){youtubedown(document.location.toString())};s.src='https://www.jwz.org/hacks/youtubedown.js';document.getElementsByTagName('body')[0].appendChild(s);})();

   Place that bookmark on your toolbar, and press it while viewing a video
   on Youtube, Vimeo, Instagram, etc.

   Created: 10-Jun-2019.


   Exported API:

     window.youtubedown_urls (url, force_fmt);
       Returns info about the underlying video URLs to download/mux.

     window.youtubedown_playlist_urls (url);
       If the given URL is a playlist or part of a playlist, returns
       the playlist's title, and the underlying (top-level) URLs of
       every video in the playlist.

     window.youtubedown (url, output_node, force_fmt);
       Downloads one or two files to your "Download" directory.

     window.youtubedown_version;



   TODO:

   - Return thumbnail image as well.

   - Because this runs synchronously, it's uninterruptible until the
     video starts downloading (except by hitting reload).

   - Figure out how to make single-part video documents present as
     "Content-Disposition: attachment".

     Currently, files are saved directly to the Downloads folder but
     with no indication of progress.  Also, the entire video document
     is buffered in browser RAM.

   - Same, but for segmented DASH video documents.

   - Figure out how to mux audio-only and video-only files together.
     Failing that, maybe pack them together into a ZIP?

   - Maybe pop up a fake in-page dialog box with multiple download
     options, so that the user can do Save Link As?

   - The Perl code sometimes downloads 5x faster by taking advantage
     of the fact that YouTube's rate-limiting does a startup burst: the
     first 10MB of a file comes fast, then it slows down, so we request
     data using multiple Range: requests of 10MB or less.  The browser
     download dialog doesn't do that, nor does XMLHttpRequest.
     Is there another way?

   - I have a vague memory that some sites require (or required) a
     faked Referer header, because that's how they enforce restrictions
     on embedding. XMLHttpRequest doesn't let you set that header.

   - Automate some test cases.
 */

(function() {

var version = '$Revision: 1.11 $'.replace(/^.*\s(\d[.\d]+)\s.*$/s, '$1');

var verbose = 2;

var webm_p = true;    // Whether to select WebM/Vorbis formats.
var log_node = null;  // Write diagnostics into this DIV if it exists.

function LOG() {
  if (! verbose) return;
  if (console && console.log) {
    arguments[0] = "youtubedown: " + arguments[0];
    // Hooray, "console.log.apply(this, arguments)" is not portable.
    if (arguments.length == 1) console.log(arguments[0]);
    else if (arguments.length == 2) console.log(arguments[0], arguments[1]);
    else if (arguments.length == 3) console.log(arguments[0], arguments[1],
                                                arguments[2]);
    else console.log(arguments[0], arguments[1], arguments[2], arguments[3]);
  }
  if (log_node) {
    var t = log_node.innerText;
    log_node.innerText = (t ? t + "\n" : "") +
      Array.from(arguments).join(' ');
  }
}

function error (err) {
  LOG ("error: " + err);
  throw new Error("youtubedown: " + err);
}

var error_whiteboard = "";
function errorI (err) {
  if (error_whiteboard) {
    err += "\n\n" + error_whiteboard;
    error_whiteboard = '';
  }
  LOG ("internal error: " + err);
  throw new Error("youtubedown: " + err);
}


// Convert any HTML entities to Unicode characters.
//
function html_unquote (str) {
  str = str.replace(/<[^<>]+>/g, '');
  var div = document.createElement('div');
  div.innerHTML = str;
  return div.innerText;
}


function fmt_size (size) {
  if (size == null || size == undefined) return "unknown size";
  return (size > 1024*1024 ? parseInt(size/(1024*1024)) + " MB" :
          size > 1024      ? parseInt(size/1024) + "KB" :
          size + " bytes");
}


function fmt_bps (bps) {   // bits per sec, not bytes
  return (bps > 1024*1024 ? parseInt(bps/(1024*1024)) + " Mbps" :
          bps > 1024      ? parseInt(bps/1024) + " Kbps" :
          bps + " bps");
}


var known_formats = {
  //
  // v=null means it's an audio-only format.
  // a=null means it's a video-only format.
  // Codecs "mp4S" and "webmS" are 3d video (left/right stereo).
  //
  //    video container  video size   audio codec    bitrate
  //
  0:   { v: 'flv',  w:  320, h:  180, a: 'mp3', abr:  64   },
  5:   { v: 'flv',  w:  320, h:  180, a: 'mp3', abr:  64   },
  6:   { v: 'flv',  w:  480, h:  270, a: 'mp3', abr:  96   },
  13:  { v: '3gp',  w:  176, h:  144, a: 'amr', abr:  13   },
  17:  { v: '3gp',  w:  176, h:  144, a: 'aac', abr:  24   },
  18:  { v: 'mp4',  w:  480, h:  360, a: 'aac', abr: 125   },
  22:  { v: 'mp4',  w: 1280, h:  720, a: 'aac', abr: 198   },
  34:  { v: 'flv',  w:  640, h:  360, a: 'aac', abr:  52   },
  35:  { v: 'flv',  w:  854, h:  480, a: 'aac', abr: 107   },
  36:  { v: '3gp',  w:  320, h:  240, a: 'aac', abr:  37   },
  37:  { v: 'mp4',  w: 1920, h: 1080, a: 'aac', abr: 128   },
  38:  { v: 'mp4',  w: 4096, h: 2304, a: 'aac', abr: 128   },
  43:  { v: 'webm', w:  640, h:  360, a: 'vor', abr: 128   },
  44:  { v: 'webm', w:  854, h:  480, a: 'vor', abr: 128   },
  45:  { v: 'webm', w: 1280, h:  720, a: 'vor', abr: 128   },
  46:  { v: 'webmS',w: 1920, h: 1080, a: 'vor', abr: 128   },
  59:  { v: 'mp4',  w:  854, h:  480, a: 'aac', abr: 128   },
  78:  { v: 'mp4',  w:  720, h:  406, a: 'aac', abr: 128   },
  82:  { v: 'mp4S', w:  640, h:  360, a: 'aac', abr: 128   },
  83:  { v: 'mp4S', w:  854, h:  240, a: 'aac', abr: 128   },
  84:  { v: 'mp4S', w: 1280, h:  720, a: 'aac', abr: 198   },
  85:  { v: 'mp4S', w: 1920, h:  520, a: 'aac', abr: 198   },
  92:  { v: 'mp4',  w:  320, h:  240, a: null              },
  93:  { v: 'mp4',  w:  640, h:  360, a: null              },
  94:  { v: 'mp4',  w:  854, h:  480, a: null              },
  95:  { v: 'mp4',  w: 1280, h:  720, a: null              },
  96:  { v: 'mp4',  w: 1920, h: 1080, a: null              },
  100: { v: 'webmS',w:  640, h:  360, a: 'vor', abr: 128   },
  101: { v: 'webmS',w:  854, h:  480, a: 'vor', abr: 128   },
  102: { v: 'webmS',w: 1280, h:  720, a: 'vor', abr: 128   },
  120: { v: 'flv',  w: 1280, h:  720, a: 'aac', abr: 128   },
  132: { v: 'mp4',  w:  320, h:  240, a: null              },
  133: { v: 'mp4',  w:  426, h:  240, a: null              },
  134: { v: 'mp4',  w:  640, h:  360, a: null              },
  135: { v: 'mp4',  w:  854, h:  480, a: null              },
  136: { v: 'mp4',  w: 1280, h:  720, a: null              },
  137: { v: 'mp4',  w: 1920, h: 1080, a: null              },
  138: { v: 'mp4',  w: 3840, h: 2160, a: null              },
  139: { v: null,                        a: 'm4a', abr:  48 },
  140: { v: null,                        a: 'm4a', abr: 128 },
  141: { v: null,                        a: 'm4a', abr: 256 },
  142: { v: 'mp4',  w:  426, h:  240, a: null               },
  143: { v: 'mp4',  w:  640, h:  360, a: null               },
  144: { v: 'mp4',  w:  854, h:  480, a: null               },
  145: { v: 'mp4',  w: 1280, h:  720, a: null               },
  146: { v: 'mp4',  w: 1920, h: 1080, a: null               },
  148: { v: null,                        a: 'aac', abr: 51  },
  149: { v: null,                        a: 'aac', abr: 132 },
  150: { v: null,                        a: 'aac', abr: 260 },
  151: { v: 'mp4',  w:   72, h:   32, a: null               },
  160: { v: 'mp4',  w:  256, h:  144, a: null               },
  161: { v: 'mp4',  w:  256, h:  144, a: null               },
  167: { v: 'webm', w:  640, h:  360, a: null               },
  168: { v: 'webm', w:  854, h:  480, a: null               },
  169: { v: 'webm', w: 1280, h:  720, a: null               },
  170: { v: 'webm', w: 1920, h: 1080, a: null               },
  171: { v: null,                        a: 'vor', abr: 128 },
  172: { v: null,                        a: 'vor', abr: 256 },
  218: { v: 'webm', w:  854, h:  480, a: null               },
  219: { v: 'webm', w:  854, h:  480, a: null               },
  222: { v: 'mp4',  w:  854, h:  480, a: null               },
  223: { v: 'mp4',  w:  854, h:  480, a: null               },
  224: { v: 'mp4',  w: 1280, h:  720, a: null               },
  225: { v: 'mp4',  w: 1280, h:  720, a: null               },
  226: { v: 'mp4',  w: 1920, h: 1080, a: null               },
  227: { v: 'mp4',  w: 1920, h: 1080, a: null               },
  242: { v: 'webm', w:  426, h:  240, a: null               },
  243: { v: 'webm', w:  640, h:  360, a: null               },
  244: { v: 'webm', w:  854, h:  480, a: null               },
  245: { v: 'webm', w:  854, h:  480, a: null               },
  246: { v: 'webm', w:  854, h:  480, a: null               },
  247: { v: 'webm', w: 1280, h:  720, a: null               },
  248: { v: 'webm', w: 1920, h: 1080, a: null               },
  249: { v: null,                        a: 'vor', abr:  50 },
  250: { v: null,                        a: 'vor', abr:  70 },
  251: { v: null,                        a: 'vor', abr: 160 },
  256: { v: null,                        a: 'm4a', abr:  97, c: 5.1},
  258: { v: null,                        a: 'm4a', abr: 191, c: 5.1},
  264: { v: 'mp4',  w: 2560, h: 1440, a: null               },
  266: { v: 'mp4',  w: 3840, h: 2160, a: null               },
  271: { v: 'webm', w: 2560, h: 1440, a: null               },
  272: { v: 'webm', w: 3840, h: 2160, a: null               },
  273: { v: 'webm', w:  854, h:  480, a: null               },
  274: { v: 'webm', w: 1280, h:  720, a: null               },
  275: { v: 'webm', w: 1920, h: 1080, a: null               },
  278: { v: 'webm', w:  256, h:  144, a: null               },
  279: { v: 'webm', w:  426, h:  240, a: null               },
  280: { v: 'webm', w:  640, h:  360, a: null               },
  298: { v: 'mp4',  w: 1280, h:  720, a: null               },
  299: { v: 'mp4',  w: 1920, h: 1080, a: null               },
  302: { v: 'webm', w: 1280, h:  720, a: null               },
  303: { v: 'webm', w: 1920, h: 1080, a: null               },
  304: { v: 'mp4',  w: 2560, h: 1440, a: null               },
  305: { v: 'mp4',  w: 3840, h: 1920, a: null               },
//308: { v: 'mp4',  w: 2560, h: 1440, a: null               },
  308: { v: 'webm', w: 2560, h: 1440, a: null               },
  313: { v: 'webm', w: 3840, h: 2160, a: null               },
  315: { v: 'webm', w: 3840, h: 2160, a: null               },
  317: { v: 'webm', w:  854, h:  480, a: null               },
  318: { v: 'webm', w:  854, h:  480, a: null               },
  327: { v: null,                        a: 'm4a', abr: 128, c: 5.1 },
  330: { v: 'webm', w: 256,  h:  144, a: null               },
  331: { v: 'webm', w: 426,  h:  240, a: null               },
  332: { v: 'webm', w: 640,  h:  360, a: null               },
  333: { v: 'webm', w: 854,  h:  480, a: null               },
  334: { v: 'webm', w: 1280, h:  720, a: null               },
  335: { v: 'webm', w: 1920, h: 1080, a: null               },
  336: { v: 'webm', w: 2560, h: 1440, a: null               },
  337: { v: 'webm', w: 3840, h: 2160, a: null               },
  338: { v: null,                     a: 'vor', abr:   4    },
  339: { v: null,                     a: 'vor', abr: 170, c: 5.1 },
  350: { v: null,                     a: 'vor', abr:  50    },
  351: { v: null,                     a: 'vor', abr:  49    },
  352: { v: null,                     a: 'vor', abr:   3    },
  357: { v: 'webm', w: 1280, h:  720, a: null               },
  358: { v: 'webm', w: 1280, h:  720, a: null               },
  359: { v: 'webm', w: 1920, h: 1080, a: null               },
  360: { v: 'webm', w: 1920, h: 1080, a: null               },
  394: { v: 'av1',  w:  256, h:  144, a: null               },
  395: { v: 'av1',  w:  426, h:  240, a: null               },
  396: { v: 'av1',  w:  640, h:  360, a: null               },
  397: { v: 'av1',  w:  854, h:  480, a: null               },
  398: { v: 'av1',  w: 1280, h:  720, a: null               },
  399: { v: 'av1',  w: 1920, h: 1080, a: null               },
  400: { v: 'av1',  w: 2560, h: 1440, a: null               },
  401: { v: 'av1',  w: 3840, h: 2160, a: null               },
  402: { v: 'av1',  w: 3840, h: 2160, a: null               },
  403: { v: 'av1',  w: 5888, h: 2160, a: null               },
};


function xmlprogress (e) {
  if (!verbose) return;
  var now = parseInt (Date.now() / 1000);
  if ((this.last_progress || 0) <= now - 5) {
    this.last_progress = now;
    LOG (parseInt (100 * event.loaded / event.total) + "%");
  }
}


// Force the URL to download, with the given default file name.
//
async function download_url (filename, url, progress_p) {

  LOG ("downloading", filename, url);

  // Create a temporary, invisible <A DOWNLOAD=...> element and click on it.
  var a = document.createElement("a");
  a.style.display = "none";
  document.body.appendChild(a);

  a.href = url;
  a.setAttribute ("download", filename);
  a.setAttribute ("target", "_blank");

  // Getting "The download attribute on anchor was ignored because its
  // href URL has a different security origin."
  // Maybe the cross-origin is that 'document' is on youtube.com but
  // HREF points to googlevideo.com?

  // This forces it to download, but at the expense of buffering the
  // entire file in RAM, and not showing any download progress indicator:
  // Grab the entire file with XHR and stuff a Blob into the HREF.
  //
  if (typeof url == 'string') {  // if it's not a Blob already
    var promise = new Promise (function (resolve, reject) {
      var conn = new XMLHttpRequest();
      conn.open('GET', url, true);
      conn.responseType = 'blob';
      conn.onload = function (e) {
        if (this.status >= 200 && this.status < 300) {
          resolve (conn.response);
        } else {
          reject({ status: this.status,
                   statusText: conn.statusText
                 });
        }
      };
      conn.onerror = function() {
        reject({ status: this.status,
                 statusText: conn.statusText
              });
      };
      if (verbose && progress_p)
        conn.onprogress = xmlprogress;
      conn.send(null);
    }).then (function (val) {
               if (verbose && progress_p)
                 xmlprogress ({loaded:1, total:1});
               url = URL.createObjectURL(val);
             });

    await promise;
    a.href = url;
  }

  a.click();
  document.body.removeChild(a);
}


// Given a list of URLs, concatenate them together and download them as
// a single document.
//
async function download_url_segments (filename, urls) {
  // This buffers the whole file in client RAM, probably twice.
  // There is no feedback about download progress.
  // It is uninterruptable except by reloading the window.
  // This is terrible, but I don't know another way.

  var promises = [];
  var count = 0;
  var resolved_count = 0;

  // Cumulative progress of all segment URLs, since they load in parallel.
  var progresses = {};
  var xmlprogress2 = function (url, e) {
    progresses[url] = e;
    var total = { loaded:0, total:0 };
    var count = 0;
    for (var e2 of Object.values(progresses)) {
      total.loaded += e2.loaded;
      total.total += e2.total;
      count++;
    }
    if (count > 1)
      xmlprogress (total);
  };

  for (var url of urls) {
    promises.push (new Promise (function (resolve, reject) {
      var conn = new XMLHttpRequest();
      conn._ytd_url = url;
      conn.open('GET', url, true);
      conn.responseType = 'blob';
      conn.download_count = count;
      conn.onload = function (e) {
        if (this.status >= 200 && this.status < 300) {
          resolved_count++;
          if (verbose > 2)
            LOG ("loaded segment " +
                 (conn.download_count+1) + '/' + urls.length,
                 url);
          resolve (conn.response);
        } else {
          reject({ status: this.status,
                   statusText: conn.statusText
                 });
        }
      };
      conn.onerror = function() {
        reject({ status: this.status,
                 statusText: conn.statusText
              });
      };
      if (verbose)
        conn.onprogress = function(e) { xmlprogress2 (this._ytd_url, e) };
      if (verbose > 2)
        LOG ("enqueueing segment " + (count+1) + '/' + urls.length, url);
      count++;
      conn.send(null);
    }));
  }

  var p2 = Promise.all (promises).then (function (values) {
    if (verbose)
      xmlprogress ({loaded:1, total:1});
    LOG ("saving " + (count+1) + " segments to \"" + filename + "\"");
    var blob = new Blob (values, { type: 'video/mp4' });
    download_url (filename, URL.createObjectURL(blob), false);
  }).catch (function (e) {
    error (e);
  });

  LOG ("awaiting " + (count+1) + " segments");
  await p2;
}


function download_video_file (filename, url) {
  if (typeof url == "object") {
    download_url_segments (filename, url);
  } else {
    download_url (filename, url, true);
  }
}


// Loads the given URL, processes redirects; retries dropped connections.
// Returns: [ http, head, body, final_redirected_url ]
//
// This is synchronous, which is typically a pretty bogus thing to do,
// but there's not really a sensible way for youtubedown to run
// asynchronously.
//
function get_url (url, referer,
                  to_file,    // unused
                  bwlimit,    // unused
                  max_bytes,
                  append_p,   // unused
                  progress_p, // unused
                  force_ranges_p,
                  extra_headers) {

  if (to_file)    error ("get_url: to_file unimplemented");
  if (bwlimit)    error ("get_url: bwlimit unimplemented");
  if (max_bytes)  error ("get_url: max_bytes unimplemented");
  if (append_p)   error ("get_url: append unimplemented");
  if (progress_p) error ("get_url: progress unimplemented");
  if (force_ranges_p) error ("get_url: force_ranges unimplemented");

  var conn = new XMLHttpRequest();
  conn.open('GET', url, false);  // synchronous

  // Hooray, XMLHttpRequest explicitly disallows this
  // if (referer) conn.setRequestHeader ('Referer', referer);

  if (extra_headers)
    for (var h in extra_headers)
      conn.setRequestHeader (h, extra_headers[h]);

  var http = '';
  var head = '';
  var body = '';

  conn.onreadystatechange = function() {
    if (this.readyState == 4) {
      http = 'HTTP/1.0 ' + this.status + ' ' + this.statusText;
      head = this.getAllResponseHeaders();
      body = this.responseText;
    }
  }

  LOG ("loading", url);
  conn.send(null);

  return [http, head, body];
}


function get_url_hdrs (url, hdrs) {
  return get_url (url, null,   // referer
                  null, // to_file
                  null, // bwlimit
                  null, // max_bytes
                  null, // append_p
                  null, // progress_p
                  null, // force_ranges_p
                  hdrs);
}


// I think we might be getting stale URLs from the browser cache.
//
function cachebuster (url) {
  return (url +
          (url.match(/\?/) ? '&' : '?') +
          // Add "&cbNNNN=NNNN" to it.
          'cb' + (Math.random() * 0xFFFFFFFF).toString().replace('.', '='));
}


function check_http_status (id, url, http, err_p) {
  if (http.match(/^HTTP\/[0-9.]+ 20\d/s)) return true;
  if (err_p) error (id + ": " + http + ": " + url);
  return false;
}


// In the Perl version, this variable is a pre-populated cache of the
// current YouTube ciphers, since determining the current cipher requires
// loading several documents. My server has a cron job that updates it
// in the code periodically, as those ciphers change at random times
// (sometimes daily, usually every few months.)
//
// In this version, the cache starts off empty, but is backed up to
// localStorage if possible.
//
var ciphers = {};


function cipher_localstorage (load_p) {
  var localstorage_key = 'youtubedown_cipher_cache';
  var n = Object.keys(ciphers).length;
  if (load_p) {
    if (n == 0) {
      try {
        ciphers = JSON.parse (localStorage.getItem (localstorage_key));
      } catch (e) {
        LOG ("load cipher cache:", e);
      }
    }
  } else {
    if (n > 0) {
      // Only save the last few, sorted by STS.
      var max = 10;
      var i = 0;
      var c2 = {};
      for (var k of Object.keys(ciphers)
             .sort(function(a,b) {
                     a = ciphers[a];
                     b = ciphers[b];
                     return (a == b ? 0 : a < b ? 1 : -1)
                   })) {
        c2[k] = ciphers[k];
        if (++i >= max) break;
      }
      try {
        localStorage.setItem (localstorage_key, JSON.stringify (c2));
      } catch (e) {
        LOG ("save cipher cache:", e);
      }
    }
  }
}


var cipher_warning_printed_p = false;
function decipher_sig (url, id, cipher, signature, via) {

  if (!cipher) return signature;

  var orig = signature;
  var s = signature.split('');

  cipher_localstorage (true);

  var c = ciphers[cipher];
  if (! c) {

    if (verbose > 2 && !cipher_warning_printed_p)
      LOG (id + ": unknown cipher " + cipher);
    c = guess_cipher (cipher, 0, cipher_warning_printed_p);
    ciphers[cipher] = c;
    cipher_warning_printed_p = true;
    cipher_localstorage (false);
  }

  c = c.replace(/([^\s])([a-z])/gs, '$1 $2');
  if (c.match (/^(\d+)\s*(.*)/si)) {
    sts = RegExp.$1;
    c = RegExp.$2;
  }

  for (var c of c.split(/\s+/)) {
    if      (c === '')              { }
    else if (c === 'r')             { s = s.reverse();  }
    else if (c.match (/^s(\d+)$/s)) { s = s.slice(RegExp.$1, s.length); }
    else if (c.match(/^w(\d+)$/s)) {
      var a = 0;
      var b = RegExp.$1 % s.length;
      [s[a], s[b]] = [s[b], s[a]];
    }
    else { errorI ("bogus cipher: " + c); }
  }

  signature = s.join('');

  var L1 = orig.length;
  var L2 = signature.length;
  if (verbose > 4 && signature !== orig) {
    LOG (id + ": translated sig, " + sts + " " + cipher + ":\n" +
         "old: " + L1 + ": " + orig + "\n" +
         "new: " + L2 + ": " + signature);
  }

  if (! (signature.match(/^[\dA-F]{30,}\.[\dA-F]{30,}$/s))) {
    error_whiteboard +=
      (id + ": suspicious signature: " + sts + " " + cipher + ":\n" +
       "url: " + url + "\n" +
       "via: " + via + "\n" +
       "old: " + L1 + ": " + orig + "\n" +
       "new: " + L2 + ": " + signature + "\n");
  }

  return signature;
}


function page_cipher_base_url (url, body) {
  body = body.replace(/\\/gs, '');
  // Sometimes but not always the "ux.js" file comes before "base.js".
  // But in the past, the file was not named "base.js"...
  var c = (body.match(/\/jsbin\/((?:html5)?player[-_][^<>\"\']+?\/base)\.js/s)
           ? RegExp.$1 : null);
  if (! c)
    c   = (body.match(/\/jsbin\/((?:html5)?player[-_][^<>\"\']+?)\.js/s)
           ? RegExp.$1 : null);
  if (c) c = c.replace(/\\/g, '');
  if (! c.match(/base$/s))
    errorI ("matched wrong cipher: " + c + ' ' + url + "\nBody:\n" + body);

  return c;
}


// Simplistic Perl /x syntax
//
// Doing anything compliated with regexps without /x really hurts.
// XRegExp plus multi-line template literals is slightly better, but still
// requires doubling up every backslash. What is this, Emacs Lisp?
//
// This function takes a multi-line string and returns a RegExp, after
// interpolating in the magic variables '$v' and '$v2'.
// Strips '#' comment to end of line, strips whitespace.
// Implements \Q\E.
//
var RX = (function() {
  var v  = '[\$a-zA-Z][a-zA-Z\\d]*';	// JS variable, 1+ characters
  var v2 = '[\$a-zA-Z][a-zA-Z\\d]?';	// JS variable, 2 characters

  // Also allow "a.b" where "a" would be used as a var.
  v  = v + '(?:\.' + v + ')?';
  v2 = v2 + '(?:\.' + v2 + ')?';

  return function(s) {
    var o = s;
    s = s.replace(/\$v\b/g, v);
    s = s.replace(/\$v2\b/g, v2);
    s = s.replace(/([^\\])#.*$/gm, '$1');  // # but not \#
    s = s.replace(/\s+/g, '');
    s = s.replace(/\\Q(.*)\\E/g, function(all, q) {
                    return q.replace(/([^-_a-z\d])/gi, '\\$1');
                  });
    try {
      return new RegExp(s, "s");  // single-line but case-sensitive
    } catch (e) {
      throw new Error(e.toString() + ': ' + o);
    }
  }})();



// Total kludge that downloads the current html5player, parses the JavaScript,
// and intuits what the current cipher is.  Normally we go by the list of
// known ciphers above, but if that fails, we try and do it the hard way.
//
function guess_cipher (cipher_id, selftest_p, nowarn) {

  // If we're in cipher-guessing mode, crank up the verbosity to also
  // mention the list of formats and which format we ended up choosing.
  // verbose = 2 if (verbose == 1 && !selftest_p);


  var url = "https://www.youtube.com/";
  var [http, head, body] = [];
  var id = '-';

  if (! cipher_id) {
    [http, head, body] = get_url (url);		// Get home page
    check_http_status ('-', url, http, 2);

    var vids = [];
    for (var m of body.match(/\/watch\?v=([^\"\'<>]+)/g)) {
      if (m.match(/v=([^\"\'<>]+)/))
        vids.push (RegExp.$1);
    }

    if (vids.count)
    errorI ("no videos found on home page " + url);

    // Get random video -- pick one towards the middle, because sometimes
    // the early ones are rental videos.
    var id = vids[parseInt(vids.count / 2)];
    url += "/watch\?v=" + id;

    [http, head, body] = get_url (url);  	// Get random video's info
    check_http_status (id, url, http, 2);

    cipher_id = page_cipher_base_url (url, body);
    if (!cipher_id)
      errorI (id + ": unparsable cipher url: " + url + "\n\nBody:\n\n" + body);
  }

  cipher_id = cipher_id.replace(/\\/gs, '');
  url = "https://s.ytimg.com/yts/jsbin/" + cipher_id + ".js";

  [http, head, body] = get_url (url);
  check_http_status (id, url, http, 2);

  var date = (head.match (/^Last-Modified:\s+(.*)$/mi)
              ? RegExp.$1 : '?');
  date = date.replace (/^[A-Z][a-z][a-z], (\d\d? [A-Z][a-z][a-z] \d{4}).*$/s,
                       '$1');


  // First, find the sts parameter:
  var sts = (body.match(/\bsts:(\d+)\b/si) ? RegExp.$1 : null);
  if (!sts) errorI (cipher_id + ": no sts parameter: " + url);


  // Since the script is minimized and obfuscated, we can't search for
  // specific function names, since those change. Instead we match the
  // code structure.
  //
  // Note that the obfuscator sometimes does crap like y="split",
  // so a[y]("") really means a.split("")


  // Find "C" in this: var A = B.sig || C (B.s)
  var fn = (body.match(RX('$v = ( $v ) \\.sig \\|\\| ( $v ) \\( \\1 \\.s \\)'))
            ? RegExp.$2 : null);

  // If that didn't work:
  // Find "C" in this: A.set ("signature", C (d));
  if (!fn)
    fn = (body.match(RX(' $v \\. set \\s* \\( "signature", \\s*   \n\
                                        ( $v ) \\s* \\( \\s* $v \\s* \\) '))
        ? RegExp.$1 : null);

  // If that didn't work:
  // Find "C" in this: (A || (A = "signature"), B.set (A, C (d)))
  if (!fn)
    fn = (body.match(RX(' "signature" \\s* \\) \\s* , \\s*   \n\
                       $v \\. set \\s* \\( \\s*   \n\
                       $v \\s* , \\s*   \n\
                       ( $v ) \\s* \\( \\s* $v \\s* \\)'))
        ? RegExp.$1 : null);

  // Wow, what!  Convert (0,window.encodeURIComponent) to just w.eUC
  body = body.replace(RX('\\(0,($v)\\)'), '$1');

  // If that didn't work:
  // Find "C" in this: A.set (B.sp, D (C (E (B.s))))
  // where "D" is "encodeURIComponent" and "E" is "decodeURIComponent"
  if (!fn)
    fn = (body.match(RX(' $v2 \\. set \\s* \\( \\s*    # A.set (  \n\
                       $v2 \\s* , \\s*                 #   B.sp, \n\
                       $v  \\s* \\( \\s*               #   D ( \n\
                       ( $v2 ) \\s* \\( \\s*           #     C ( \n\
                       $v  \\s* \\( \\s*               #       E ( \n\
                       $v2 \\s*                        #         B.s \n\
                       \\) \\s* \\) \\s* \\) \\s* \\)  #         ))))'))
        ? RegExp.$1 : null);


  // If that didn't work:
  // Find "C" in this: A.set (B, C (d))
  // or this: A.set (B.sp, C (B.s))
  if (!fn)
    fn = (body.match(RX(' $v2 \\. set \\s* \\( \\s*   \n\
                       $v2 \\s* , \\s*   \n\
                       ( $v2 ) \\s* \\( \\s* $v2 \\s* \\) \\s* \\)'))
        ? RegExp.$1 : null);


  if (!fn)
    errorI (cipher_id + ": unparsable cipher js: " + url);
  // Congratulations! If the above error fired, start looking through 'url'
  // for a consecutive series of 2-arg function calls ending with a number.
  // The containing function is the decipherer, and its name goes in 'fn'.


  // Find body of function C(D) { ... }
  // might be: var C = function(D) { ... }
  // might be:   , C = function(D) { ... }
  var fn2 = (body.match(RX('\n\
               \\b function \\s+ \\Q' + fn + '\\E \\s* \\( $v \\)   \n\
                          \\s* { ( .*? ) }'))
             ? RegExp.$1 : null);
  if (!fn2)
    fn2 = (body.match(RX('(?: \\b var \\s+ | [,;] \\s* )   \n\
                      \\Q' + fn + '\\E \\s* = \\s* function \\s* \\( $v \\) \n\
                      \\s* { ( .*? ) }'))
           ? RegExp.$1 : null);

  if (!fn2)
    errorI (cipher_id + ": unparsable fn \"" + fn + "\"");

  fn = fn2;

  error_whiteboard += "fn: " + fn2 + "\n";

  // They inline the swapper if it's used only once.
  // Convert "var b=a[0];a[0]=a[63%a.length];a[63]=b;" to "a=swap(a,63);".
  fn2 = fn2.replace(RX('   \n\
            var \\s ( $v ) = ( $v ) \\[ 0 \\];   \n\
            \\2 \\[ 0 \\] = \\2 \\[ ( \\d+ ) % \\2 \\. length \\];   \n\
            \\2 \\[ \\3 \\]= \\1 ;'),
                    '$2=swap($2,$3);');

  var cipher = [];
  for (var c of fn2.split (/\s*;\s*/)) {

    // Typically the obfuscator gives member functions names like 'XX.YY',
    // but in the case where 'YY' happens to be a reserved word, like 'do',
    // it will instead emit 'XX["YY"]'.
    //
    c = c.replace(RX(' ^ ( $v ) \\[" ( $v ) "\\] /'), '$1.$2');

    if        (c.match(RX('^ ( $v ) = \\1 . $v \\(""\\) $'))) {
      // A=A.split("");
    } else if (c.match(RX('^ ( $v ) = \\1 .  $v \\(\\)  $'))) {
      // A=A.reverse();
      error_whiteboard += "fn: r: " + RegExp.$1 + "\n";
      cipher.push("r");
    } else if (c.match(RX('^ ( $v ) = \\1 . $v \\( (\\d+) \\) $'))) {
      // A=A.slice(N);
      error_whiteboard += "fn: s: " + RegExp.$1 + "\n";
      cipher.push("s" + RegExp.$2);

    } else if (c.match(RX('^ ( $v ) = ( $v ) \\( \\1 , ( \\d+ ) \\) $')) ||
               c.match(RX('^ (    )   ( $v ) \\( $v , ( \\d+ ) \\) $'))) {
      // A=F(A,N);
      // F(A,N);
      var f = RegExp.$2;
      var n = RegExp.$3;
      f = f.replace(/^.*\./gs, '');  // C.D => D
      // Find function D, of the form: C={ ... D:function(a,b) { ... }, ... }
      // Sometimes there will be overlap: X.D and Y.D both exist, and the
      // one we want is the second one. So assume the one we want is simple
      // enough to not contain any {} inside it.
      var fn3 = (body.match(RX(' \\b "? \\Q' + f + '\\E "? : \\s*   \n\
                               function \\s* \\( [^(){}]*? \\) \\s*   \n\
                                ( \\{ [^{}]+ \\} )'))
                 ? RegExp.$1 : null);
      if (!fn3) {
        fn = fn.replace(/;/gs, ';\n\t    ');
        error ("unparsable: function \"" + f + "\" not found\n\tin: " + fn);
      }
      // Look at body of D to decide what it is.
      if (fn3.match(RX(' var \\s ( $v ) = ( $v ) \\[ 0 \\]; '))) {  // swap
        error_whiteboard += "fn3: w: " + f + ": " + fn3 + "\n";
        cipher.push("w" + n);
      } else if (fn3.match(RX(' \\b $v \\. reverse\\( '))) {        // reverse
        error_whiteboard += "fn3: r: " + f + ": " + fn3 + "\n";
        cipher.push("r");
      } else if (fn3.match(RX(' return \\s* $v \\. slice ')) ||     // slice
                 fn3.match(RX(' \\b $v \\. splice '))) {            // splice
        error_whiteboard += "fn3: s: " + f + ": " + fn3 + "\n";
        cipher.push("s" + n);
      } else {
        fn = fn.replace(/;/gs, ';\n\t    ');
        errorI ("unrecognized cipher body " + f + "(" + n + ") = " +
                fn3 + "\n\tin: " + fn);
      }
    } else if (c.match(RX('^ return \\s+ $v \\. $v \\(""\\) $'))) {
      // return A.join("");
    } else {
      fn = fn.replace(/;/gs, ';\n\t    ');
      errorI (cipher_id + ": unparsable: " + c + "\n\tin: " + fn);
    }
  }
  var cipher = sts + " " + cipher.join(' ');

  error_whiteboard += "cipher: " + cipher + "\n";

  if (selftest_p) {
    if (ciphers[cipher_id]) return cipher;
    if (verbose < 2) verbose = 2;
  }

  if (verbose > 1 && !nowarn) {
    var c2 = "  '" + cipher_id + "' => '" + cipher + "',";
    var c2 = c2 + "\t\t# " + date;
    // if (selftest_p == 2) auto_update(c2);
    LOG ("current cipher is: " + c2);
  }

  return cipher;
}


// Replace the signature in the URL, deciphering it first if necessary.
//
function apply_signature (id, fmt, url, cipher, sig, via) {
  if (sig) {
    if (cipher) {
      var o = sig;
      sig = decipher_sig (url, fmt ? id + "/" + fmt : id, cipher, sig, via);
      if (o !== sig) {
        var n = sig;
        var [a, b] = o.split(/\./);
        var [c, d] = sig.split(/\./);
        if (!b) { a = o; b = ''; }
        if (!d) { c = sig; d = ''; }
        var L1 = o.length + ' ' + a.length + ' ' + b.length;
        var L2 = sig.length + ' ' + c.length + ' ' + d.length;
        var s = "cipher: " + cipher + "\n" + L1 + ": " + o + "\n" + L2 +
          ": " + n;
        if (!fmt) fmt = '?';
        if (verbose > 3) {
          LOG (id + ": deciphered and replaced signature");
        }
      }
    }

    if (url.match(/^(.*)\/s\/[^\/]+(.*)$/s)) {
      // DASH /s/ => /signature/
      url = RegExp.$1 + "/signature/" + sig + RegExp.$2;
    } else if (url.match(/\?/s)) {
      // Default is to do "signature=SIG" but if there is "sp=XXX"
      // in the url_map, that means it goes in "XXX=SIG" instead
      // in the video URL.
      var sig_tag = (via.match(/&sp=([^&]+)/s) ? RegExp.$1 : 'signature');
      url = url.replace (RX('& ( signature | sig |      \n\
                                 \Q ' + sig_tag + '\E   \n\
                               ) = [^&]+'),
                         '');
      url += '&' + sig_tag + '=' + sig;
    } else {
      errorI ("unable to splice signature: " + url);
    }
  }
  return url;
}


// Convert the text of a Youtube urlmap field into a structure.
// Apply signatures to enclosed URLs as necessary.
// Returns null if the signatures could not be applied.
// If 'into' is provided, inserts the new items there, but does not overwrite
// existing ones.
//
// Returns the number of formats parsed (including redundant ones).
//
function youtube_parse_urlmap (id, urlmap, cipher, into) {

  var cipher_printed_p = false;

  var count = 0;
  for (var mapelt of urlmap.split(/,/)) {
    // Format used to be: "N|url,N|url,N|url"
    // Now it is: "url=...&quality=hd720&fallback_host=...&type=...&itag=N"
    var [k, v, e, sig, sig2, sig3, w, h, size] = [];
    if (mapelt.match (/^(\d+)\|(.*)/s)) {
      k = RegExp.$1;
      v = RegExp.$2;
    } else if (mapelt.match(/^[a-z][a-z\d_]*=/s)) {

      sig  = (mapelt.match (/\bsig=([^&]+)/s)	// sig= when un-ciphered.
              ? RegExp.$1 : null);
      sig2 = (mapelt.match (/\bs=([^&]+)/s)	// s= when enciphered.
              ? RegExp.$1 : null);
      sig3 = (mapelt.match (/\/s\/([^\/?&]+)/s)	// /s/XXX/ in DASH.
              ? RegExp.$1 : null);

      k = (mapelt.match(/\bitag=(\d+)/s) ? RegExp.$1 : null);
      v = (mapelt.match(/\burl=([^&]+)/s) ? RegExp.$1 : null);
      if (v) v = decodeURIComponent(v);

      size  = (v.match(/\bclen=([^&]+)/s) ? RegExp.$1 : null);
      if (v.match(/\bsize=(\d+)x(\d+)/s)) {
        w = RegExp.$1;
        h = RegExp.$1;
      }

      q = (mapelt.match(/\bquality=([^&]+)/s) ? RegExp.$1 : null);
      t = (mapelt.match(/\btype=([^&]+)/s) ? RegExp.$1 : null);
      if (t) t = decodeURIComponent(t);
      if (q && t) {
        e = "\t" + q + ", " + t;
      } else if (t) {
        e = t;
      }
      if (e) e = decodeURIComponent(e);
    }

//    if (!v && urlmap.match(/\bconn=rtmpe%3A/s))
//      // There was no indiciation in get_video_info that this is an RTMPE
//      // stream, so it took us several retries to fail here.
//      error (id + ": can't download RTMPE DRM videos");

    if (!k) errorI (id + ": unparsable urlmap entry: no itag: " + mapelt);
    if (!v) errorI (id + ": unparsable urlmap entry: no url: " + mapelt);

    var ct = (e.match(/\b((audio|video|text|application)\/[-_a-z\d]+)\b/si)
              ? RegExp.$1 : null);

    v = v.replace(/^.*?\|/s, '');  // VEVO

    if ((sig2 || sig3) && !cipher)
      errorI (id + ": enciphered URL but no cipher found: " + v);

    if (verbose > 1 && !cipher_printed_p) {
      LOG (id + ": " +
           ((sig2 || sig3) ? "enciphered" : "non-enciphered") +
           ((sig2 || sig3) ? " (" + (cipher || 'NONE') + ")" :
            (cipher ? " (" + cipher + ")" : "")));
      cipher_printed_p = true;
    }

    // Apply the signature to the URL, deciphering it if necessary.
    //
    // The "use_cipher_signature" parameter is as lie: it is sometimes true
    // even when the signatures are not enciphered.  The only way to tell
    // is if the URLs in the map contain "s=" instead of "sig=".
    //
    // If we loaded get_video_info with the "sts" parameter, meaning we told
    // it what cipher to use, then the returned URLs have that cipher, and
    // all is good.  However, if we had omitted the "sts" parameter, then
    // the URLs come back with some unknown cipher (it's not the last cipher
    // in the list, for example) so we can't decode it.
    //
    // So in the bad old days, we didn't use "sts", and when we got an
    // enciphered video, we had to scrape the HTML to find the real cipher.
    // This had the shitty side effect that when a video was both enciphered
    // and was "content warning", we couldn't download it at all.
    //
    // But now that we always pass "sts" to get_video_info, this isn't a
    // problem any more.  I think that in this modern world, we never actually
    // need to scrape HTML any more, because we should always know a working
    // cipher ahead of time.
    //
    // Aug 2018: Nope, we now scrape HTML every time because that's the only
    // way to reliably get dashmpd URLs that work.
    //
    v = apply_signature (id, k, v,
                         (sig2 || sig3) ? cipher : null,
                         decodeURIComponent (sig || sig2 || sig3 || ''),
                         mapelt);

    // Finally! The "ratebypass" parameter turns off rate limiting!
    // But we can't add it to a URL that signs the "ratebypass" parameter.
    //
    if (! (v.match(/sparams=[^?&]*ratebypass/) ||
           v.match(/sparams\/[^\/]*ratebypass/))) {
      if (v.match(/\?/s)) {
        v += '&ratebypass=yes';
      } else if (v.match(/\/itag\//s)) {   // dashmpd-style.
        v += (v.match(/\/$/s) ? '' : '/') + 'ratebypass/yes/';
      }
    }

    if (verbose > 3) LOG("\t\t" + k + "\t" + v + "\t" + e);

    if (v.match(/&live=(1|yes)\b/gs)) {
      // We need to get the segments from the DASH manifest instead:
      // this URL is only the first segment, a few seconds long.
      if (verbose > 2) LOG (id + ": skipping fmt " + k);
      continue;
    }

    var v = { fmt: k,
              url: v,
              content_type: ct,
              w: w,
              h: h,
              size: size,
    };

    if (!into[k]) {
      into[k] = v;
      if (verbose > 2) LOG (id + ": found fmt " + k);
    }

    count++;
  }

  return count;
}


// There are two ways of getting the underlying video formats from youtube:
// parse it out of the HTML, or call get_video_info.
//
// We have to do both, because they all fail in different ways at different
// times, so we try a bunch of things and append together any results we find.
// The randomness leading to this crazy approach includes but is not limited
// to:
//
//  - The DASH URL in the HTML always works, but sometimes the DASH URL in
//    get_video_info does not -- the latter appears to use a different cipher.
//
//  - There are 4 different ways of invoking get_video_info, and sometimes
//    only one of them works. E.g., sometimes the "el=" option is needed to
//    retrieve info, but sometimes it *prevents* you from retrieving info.
//    E.g., "info" and "embedded" sometimes give geolocation errors, and
//    yet are the only way to bypass the age gate.
//
//  - Sometimes all formats are present in get_video_info, but sometimes only
//    the 720p and lower resolutions are there, and higher resolutions are
//    only listed in the DASH URL.
//
//  - Sometimes the DASH URL pointed to from the HTML and the DASH URL pointed
//    to by get_video_info have different sets of formats in them.
//
//  - And sometimes there are no DASH URLs.


// Parses a dashmpd URL and inserts the contents into $fmts
// as per youtube_parse_urlmap.
//
function youtube_parse_dashmpd (id, url, cipher, into) {

  // I don't think this is needed.
  // url += '?disable_polymer=true';

  // Some dashmpd URLs have /s/NNNNN.NNNNN enciphered signatures in them.
  // We have to replace them with /signature/MMMMM.MMMMM or we can't read
  // the manifest file. The URLs *within* the manifest will also have
  // signatures on them, but ones that (I think?) do not need to be
  // deciphered.
  //
  if (url.match(/\/s\/([^\/]+)/s)) {
    var sig = RegExp.$1;
    if (verbose > 1) LOG (id + " DASH manifest enciphered");
    url = apply_signature (id, null, url, cipher, sig, '');
    if (verbose > 1) LOG (id + ": DASH manifest deciphered");
  } else {
    if (verbose > 2) LOG (id + ": DASH manifest non-enciphered");
  }

  var count = 0;
  url = cachebuster (url);
  var [http2, head2, body2] = get_url (url);
  check_http_status (id, url, http2, 2);

  // Nuke the subtitles: the Representations inside them aren't useful.
  body2 = body2.replace(
            /<AdaptationSet mimeType=[\"\']text\/.*?<\/AdaptationSet>/gsi,
            '');

  var reps = body2.split(/<Representation\b/si);
  reps.shift();
  for (var rep of reps) {
    var k    = (rep.match(/id=[\'\"](\d+)/si)            ? RegExp.$1 : null);
    var url  = (rep.match(/<BaseURL\b[^<>]*>([^<>]+)/si) ? RegExp.$1 : null);
    var type = (rep.match(/\bcodecs="(.*?)"/si)          ? RegExp.$1 : null);
    var w    = (rep.match(/\bwidth="(\d+)"/si)           ? RegExp.$1 : null);
    var h    = (rep.match(/\bheight="(\d+)"/si)          ? RegExp.$1 : null);
    var segs = (rep.match(/<SegmentList[^<>]*>(.*?)<\/SegmentList>/si)
                ? RegExp.$1 : null);
    var size;
    type = ((w && h ? 'video/mp4' : 'audio/mp4') +
            ';+codecs="' + type + '"');

    if (segs) {
      var url0 = (segs.match(/<Initialization\s+sourceURL="(.*?)"/si)
                  ? RegExp.$1 : null);

      // get a list of all sub-matches.
      var re = /<SegmentURL\s+media="(.*?)"/gsi;
      var urls = [];
      while (true) {
        var m = re.exec (segs);
        if (!m) break;
        urls.push(m[1]);
      }

      if (url0) urls.unshift (url0);
      urls = urls.map (function(a) { return url + a; });
      url = urls;

      if (url0)  // Not always present
        size = url0.match(/clen\/(\d+)/si ? RegExp.$1 : null);
    }

    var v = { fmt: k,
              url: url,
              content_type: type,
              dashp: true,
              w: w,
              h: h,
              size: size,
              abr: known_formats[k].abr,
    };

    // Sometimes the DASH URL for a format works but the non-DASH URL is 404.
    var prefer_dash_p = true;
    var old = into[k];
    if (prefer_dash_p && old && !old['dashp'])
      old = null;
    if (!old) {
      into[k] = v;
      if (verbose > 2)
        LOG (id + ": found fmt " + k +
             (typeof (url) === 'object' ? " (" + url.length + " segs)" : ""));
    }
    count++;
  }

  return count;
}


// For some errors, we know there's no point in retrying.
//
var blocked_re = ['(available|blocked it) in your country',
                  'copyright (claim|grounds)',
                  'removed by the user',
                  'account.*has been terminated',
                  'has been removed',
                  'has not made this video available',
                  'has closed their YouTube account',
                  'is not available',
                  'is unavailable',
                  'is not embeddable',
                  'can\'t download rental videos',
                  'livestream videos',
                  'invalid parameters',
                  'RTMPE DRM',
                  'Private video\?',
                  'piece of shit',
                  'you are a human',
                  '^[^:]+: exists: ',
                  '\b429 Too Many Requests',
                  ].join('|');


// Scrape the HTML page to extract the video formats.
// Populates 'fmts' and returns "error_message".
//
function load_youtube_formats_html (id, url, fmts) {

  var oerror = '';
  var err = '';

  url = cachebuster (url);
  var [http, head, body] = get_url (url);

  var title = (body.match(/<title>\s*(.*?)\s*<\/title>/si)
               ? RegExp.$1 : '');

  // #### Perl doesn't need to do this, why?
  if (body.match(/"title":"(.*?)"/s)) title = RegExp.$1;

  title = munge_title (html_unquote (title));

  get_youtube_year (id, body);  // Populate cache so we don't load twice.

  var unquote_p = true;
  var args = (body.match(/'SWF_ARGS' *: *{(.*?)}/s) ? RegExp.$1 : null);

  if (! args) {    // Sigh, new way as of Apr 2010...
    args = (body.match(/var swfHTML = [^\"]*\"(.*?)\";/si)
            ? RegExp.$1 : null);
    if (args) args = args.replace(/\\/gs, '');
    if (args) args = (args.match(/<param name="flashvars" value="(.*?)">/si)
                      ? RegExp.$1 : null);
    if (args) args = (args.match(/fmt_url_map=([^&]+)/si)
                      ? RegExp.$1 : null);
    if (args) args = "\"fmt_url_map\": \"" + args + "\"";
  }
  if (! args) {    // Sigh, new way as of Aug 2011...
    args = (body.match(/'PLAYER_CONFIG':\s*{(.*?)}/s) ? RegExp.$1 : null);
    if (args) args = args.replace(/\\u0026/gs, '&');
    unquote_p = false;
  }
  if (! args) {    // Sigh, new way as of Jun 2013...
    args = (body.match(/ytplayer\.config\s*=\s*{(.*?)};/s) ? RegExp.$1 : null);
    if (args) args = args.replace(/\\u0026/gs, '&');
    unquote_p = true;
  }

  if (! args) {
    // Try to find a better error message
    if (body.match(RX('<( div | h1 ) \\s+   \n\
                           (?: id | class ) =    \n\
                          "(?: error-box |   \n\
                               yt-alert-content |   \n\
                               unavailable-message )"   \n\
                          [^<>]* > \\s*    \n\
                          ( [^<>]+? ) \\s*   \n\
                          </ \\1 >')))
      err = RegExp.$2;
    if (!err && body.match(/large volume of requests/))
      err = "Rate limited: CAPCHA required";
    if (err) {
      var err2 = (body.match (/<div class="submessage">(.*?)<\/div>/si)
                  ? RegExp.$1 : "");
      if (err2) {
        err2 = err2.replace (/<button.*$/gs, '');
        err2 = err2.replace (/<[^<>]*>/gs, '');
        err += ": " + err2;
      }
      err = err.replace(/^"[^\"\n]+"\n/s, '');
      err = err.replace(/^&quot;[^\"\n]+?&quot;\n/s, '');
      err = err.replace(/\s+/gs, ' ');
      err = err.replace(/^\s+|\s+$/s, '');
      err = err.replace(/\.(: )/gs, '$1');
      err = err.replace(/\.$/gs, '');

      if (title) err = err + " (" + title + ")";

      oerror = err;
      http = 'HTTP/1.0 404';
    }
  }

  if (oerror) oerror = oerror.replace (/<.*?>/gs, '');

  if (oerror) oerror = oerror.replace (/ \(YouTube\)$/s, '');

  // Sometimes Youtube returns HTTP 404 pages that have real messages in them,
  // so we have to check the HTTP status late. But sometimes it doesn't return
  // 404 for pages that no longer exist. Hooray.

  if (oerror.match(new RegExp(blocked_re, 'si')))
    http = 'HTTP/1.0 404';
  if (! check_http_status (id, url, http, false))
    err = http + ": " + oerror;
  if (!args && !err)
    err = "no ytplayer.config" + oerror;

  var cipher = page_cipher_base_url (url, body);

  var kind;
  var kind2;
  var urlmap;
  var urlmap2;

  // hlsvp are m3u8u files, but that data always seems to also be present
  // in dash, so I haven't bothered parsing those.

  var count = 0;
  for (var key of [//'hlsvp',
                   'fmt_url_map',
                   'fmt_stream_map', // VEVO
                   'url_encoded_fmt_stream_map', // Aug 2011
                   'adaptive_fmts',
                   'dashmpd',
                   'player_response']) {
    var v = (args.match(new RegExp('"' + key + '": *"(.*?[^\\\\])"', 's'))
             ? RegExp.$1 : '');
    if (v == '",') v = '';
    v = v.replace(/\\/gs, '');
    if (!v) continue;
    if (verbose > 2) LOG (id + " HTML: found " + key);
    if (key === 'dashmpd' || key === 'hlsvp') {
      count += youtube_parse_dashmpd (id + " HTML", v, cipher, fmts);
    } else if (key === 'player_response') {
      v = (v.match(/"dashManifestUrl": *"(.*?[^\\])"/s) ? RegExp.$1 : '');
      // This manifest sometimes works when the one in get_video_info doesn't.
      if (v) count += youtube_parse_dashmpd (id + " HTML", v, cipher, fmts);
    } else {
      count += youtube_parse_urlmap (id + " HTML", v, cipher, fmts);
    }
  }

  if (!fmts['title'])  fmts['title']  = title;
  if (!fmts['cipher']) fmts['cipher'] = cipher;
  return err;
}


// Loads various versions of get_video_info to extract the video formats.
// Populates 'fmts' and returns error_message.
//
function load_youtube_formats_video_info (id, url, fmts) {

  var cipher = fmts['cipher'];
  var sts = null;

  if (cipher) {
    var c = ciphers[cipher];
    if (! c) {
      if (verbose > 2 && !cipher_warning_printed_p)
        LOG ("WARNING: " + id + ": unknown cipher " + cipher);
      c = guess_cipher (cipher, 0, cipher_warning_printed_p);
      ciphers[cipher] = c;
      cipher_localstorage (false);
    }
    if (c.match(/^\s*(\d+)\s/si))
      sts = RegExp.$1 ;
    if (!sts) errorI (id + ": cipher" + ": no sts");
  }

  var info_url_1 = ('https://www.youtube.com/get_video_info' +
                    "?video_id=" + id +
                    (sts ? '&sts=' + sts : '') +

                    // I don't think any of these are needed.
                    // '&ps=default' +
                    // '&hl=en' +
                    // '&disable_polymer=true' +
                    // '&gl=US' +

                    // Avoid "playback restricted" or "content warning".
                    // They sniff this referer for embedding.
                    '&eurl=' +
                    encodeURIComponent ('https://youtube.googleapis.com/v/' +
                                        id)
                   );

  // Sometimes the 'el' arg is needed to avoid "blocked it from display
  // on this website or application". But sometimes, including it *causes*
  // "sign in to confirm your age". So try it with various options.
  //
  // Note that each of these can return a different error message for the
  // same unloadable video, so arrange them with better error last:
  //
  //   "":         "This video is unavailable"
  //   embedded:   "This video is unavailable"
  //   info:       "Invalid parameters"
  //   detailpage: "This video has been removed by the user"
  //
  var extra_parameters = ['', '&el=embedded', '&el=info', '&el=detailpage'];

  var [title, body, embed_p, rental, live_p] = [];

  var err = null;
  var done = false;

  // The retries here are because sometimes we get HTTP 200, but the body
  // of the document contains fewer parameters than it should; reloading
  // sometimes fixes it.

//  var retries = 5;
  var retries = 1;
  while (retries--) {
    for (var extra of extra_parameters) {

      var info_url = info_url_1 + extra;

      // #### We could load all 4 of these documents in parallel instead
      //      of series using Promise.all and await.

      var [http, head] = [];
      url = cachebuster (url);
      [http, head, body] = get_url (info_url);
      var err2 = (check_http_status (id, url, http, 0) ? null : http);
      if (!err) err = err2;

      if (!title)
        title  = (body.match(/&title=([^&]+)/si)         ? RegExp.$1 : null);
      embed_p  = (body.match(/&allow_embed=([^&]+)/si)   ? RegExp.$1 : null);
      rental   = (body.match(/&ypc_vid=([^&]+)/si)       ? RegExp.$1 : null);
      live_p   = (body.match(/&live_playback=([^&]+)/si) ? RegExp.$1 : null);
      if (!embed_p &&
          body.match(/on[\s+]other[\s+]websites/s))
        embed_p = false;

      if (!live_p)
        err = "can't download livestream videos";
      // "player_response" contains JSON:
      //   "playabilityStatus":{
      //     "status":"LIVE_STREAM_OFFLINE",
      //     "reason":"Premieres in 10 hours",

      var count = 0;
      for (var key of [//'hlsvp',
                       'fmt_url_map',
                       'fmt_stream_map', // VEVO
                        'url_encoded_fmt_stream_map', // Aug 2011
                       'adaptive_fmts',
                       'dashmpd']) {
        var v = (body.match(new RegExp('[?&]' + key + '=([^&?]+)', 'si'))
                 ? RegExp.$1 : null);
        if (!v) continue;
        v = decodeURIComponent (v);
        v = v.replace(/\\/gs, '');

        if (verbose > 1)
          LOG (id + " VI: found " + key +
               (embed_p !== null
                ? (embed_p ? " (embeddable)" : " (non-embeddable)")
                : ""));
        if (key === 'dashmpd' || key === 'hlsvp') {
          count += youtube_parse_dashmpd (id + " VI", v, cipher, fmts);
        } else {
          count += youtube_parse_urlmap (id + " VI", v, cipher, fmts);
        }
      }

      done = (count >= 3 && title);

      if (body.match(/\bstatus=fail\b/si) &&
          body.match(/\breason=([^?&]+)/si))
        err = decodeURIComponent (RegExp.$1)
    }
    if (done) break;
    //sleep (1); // for retries
  }

  if (err) {
    err = err.replace(/<[^<>]+>/gs, '');
    err = err.replace(/\n/gs, ' ');
    err = err.replace(/\s*Watch on YouTube\.?/gs, ''); // FU
  }

  if (err && (embed_p !== null && !embed_p))
    err = "video is not embeddable";

  if (!body) body = '';
  if (!title)
    title = (body.match(/&title=([^&]+)/si) ? RegExp.$1 : null);
  if (!title && !err)
    errorI (id + ": no title in " + info_url_1);
  if (title)
    title = decodeURIComponent(title)

  if (!err) {
    err = (body.match(/reason=([^&]+)/s) ? RegExp.$1 : '');
    if (err) {
      err = decodeURIComponent(err);
      err = err.replace(/^"[^\"\n]+"\n/s, '');
      err = err.replace(/\s+/gs, ' ');
      err = err.replace(/^\s+|\s+$/s, '');
      err = " (\"" + err + "\")";
    }
  }

  if (err) err = err.replace(/ \(YouTube\)$/s, '');

  if (err && rental) err += ': rental video';

  if (err && rental) {
    error ("can't download rental videos, but the preview can be " +
           "downloaded at " +
           "https://www.youtube.com/watch?v=" + rental);
  }

  if (!fmts['title'])  fmts['title']  = title;
  if (!fmts['cipher']) fmts['cipher'] = cipher

  return err;
}


// Returns:
//  [ title: "T",
//    year: "Y",
//    0: [ ...video info... ],
//
function load_youtube_formats (id, url, size_p) {

  var fmts = {};
  var err = null;
  var err2 = null;

  // Scrape the HTML page before loading get_video_info because the
  // DASH URL in the HTML page is more likely to work than the one
  // returned by get_video_info.

  // I don't think any of these are needed.
  // url += join('&',
  //              'has_verified=1',
  //              'bpctr=9999999999',
  //              'hl=en',
  //              'disable_polymer=true');

  err2 = load_youtube_formats_html (id, url, fmts);
  err = err || err2;

  err2 = load_youtube_formats_video_info (id, url, fmts);
  err = err || err2;

  // It's rare, but there can be only one format available.
  // Keys: 18, cipher, title.

  if (Object.keys(fmts).count < 3) {
    if (err) error (id + ": " + err);
    errorI (id + ": no formats available: " + err);
  }

  return fmts;
}


// Returns:
//  [ title => "T",
//    N => [ ...video info... ],
//    M => [ ...video info... ], ... ]
//
function load_vimeo_formats (id, url, size_p) {

  // Vimeo's new way, 3-Mar-2015.
  // The "/NNNN?action=download" page no longer exists. There is JSON now.

  // This URL is *often* all that we need:
  //
  var info_url = ("https://player.vimeo.com/video/" + id + "/config" +
                  "?bypass_privacy=1");  // Not sure if this does anything

  // But if we scrape the HTML page for the version of the config URL
  // that has "&s=XXXXX" on it (some kind of signature, I presume) then
  // we *sometimes* get HD when we would not have gotten it with the
  // other URL:
  //
  var [http, head, body] = get_url (url);
  // Don't check status: sometimes the info URL is on the 404 page!
  // Maybe this happens if embedding is disabled.

  if (body.match(/([^<>.:]*verify that you are a human[^<>.:]*)/si)) {
    error (id + ": " + http + RegExp.$1);  // info_url will not succeed.
  }

  var obody = body;  // Might be a better error message in here.

  body = body.replace(/\\/gs, '');
  if (body.match(
               /(\bhttps?:\/\/[^\/]+\/video\/\d+\/config\?[^\s\"\'<>]+)/si)) {
    info_url = html_unquote (RegExp.$1);
  } else {
    if (verbose > 1) LOG (id + ": no info URL");
  }

  var referer = url;

  // Test cases:
  //
  //   https://vimeo.com/120401488
  //     Has a Download link on the page that lists 270p, 360p, 720p, 1080p
  //     The config url only lists 270p, 360p, 1080p
  //   https://vimeo.com/70949607
  //     No download link on the page
  //     The config URL gives us 270p, 360p, 1080p
  //   https://vimeo.com/104323624
  //     No download link
  //     Simple info URL gives us only one size, 360p
  //     Signed info URL gives us 720p and 360p
  //   https://vimeo.com/117166426
  //     A private video
  //   https://vimeo.com/88309465
  //     "HTTP/1.1 451 Unavailable For Legal Reasons"
  //     "removed as a result of a third-party notification"
  //   https://vimeo.com/121870373
  //     A private video that isn't 404 for some reason
  //   https://vimeo.com/83711059
  //     The HTML page is 404, but the simple info URL works,
  //     and the video is downloadable anyway!
  //   https://vimeo.com/209
  //     Yes, this is a real video.  No "h264" in "files" metadata,
  //     only .flv as "vp6".
  //   https://www.vimeo.com/142574658
  //     Only has "progressive" formats, not h264.  Downloads fine though.

  info_url = cachebuster (info_url);
  [http, head, body] = get_url (info_url, referer);

  var err = null;
  if (!check_http_status (id, info_url, http, 0)) {
    err = (body.match(RX(' \\{ "message" : \\s* " ( .+? ) " , '))
           ? RegExp.$1 : null);
    if (err && err.match(/privacy setting/si))
      err = "Private video";
    if (body.match(/([^<>.:]*verify that you are a human[^<>.:]*)/si))
      err = RegExp.$1;
    err = http + (err ? ": " + err : "");
  } else {
    http = '';  // 200
  }

  var title = (body.match(RX('    "title" : \\s* " (.+?) ", '))
               ? RegExp.$1 : null);
  var files0 = (body.match(RX('\\{ "h264"  : \\s* \\{ ( .+? \\} ) \\} , '))
                ? RegExp.$1 : null);
  var files1 = (body.match(RX('\\{ "vp6"   : \\s* \\{ ( .+? \\} ) \\} , '))
                ? RegExp.$1 : null);
  var files2 = (body.match(RX('"progressive" : \\s* \\[ ( .+? \\] ) \\} '))
                ? RegExp.$1 : null);

  var files    = (files0 || '') + (files1 || '') + (files2 || '');

  // Sometimes we get empty-ish data for "Private Video", but HTTP 200.
  if (!err && !title && !files)
    err = "No video info (Private video?)";

  if (err) {
    if (obody) {
      // The HTML page might provide an explanation for the error.
      var err2 = (obody.match(RX(' exception_data \\s* = \\s* { [^{}]*   \n\
                                 "notification" \\s* : \\s* " (.*?) ",'))
                  ? RegExp.$1 : null);
      if (err2) {
        err2 = err2.replace(/\\n/gs, "\n");    // JSON
        err2 = err2.replace(/\\/gs, "");
        err2 = err2.replace(/<[^<>]*>/gs, ""); // Lose tags
        err2 = err2.replace(/^\s+/gs, "");
        err2 = err2.replace(/\n.*$/gs, "");    // Keep first para only.
        if (err2) err += " " + err;
      }
    }

    if (http || err.match(/Private/s))
      error (id + ": " + err);
    errorI (id + ": " + err);
  }

  var fmts = {};

  if (files) {
    if (!title) errorI (id + ": no title");
    fmts['title'] = title;
    var i = 0;
    for (var f of files.split (/\},?\s*/)) {
      if (f.length < 50) continue;
      var fmt  = (f.match(RX('^ \\" (.+?) \\": ')) ? RegExp.$1 : null);
      if (!fmt)
          fmt  = (f.match(RX('^ \\{ "profile": (\\d+)')) ? RegExp.$1 : null);
      var url2 = (f.match(RX(' "url"   : \\s* " (.*?) "')) ? RegExp.$1 : null);
      var w    = (f.match(RX(' "width" : \\s*   (\d+)')) ? RegExp.$1 : null);
      var h    = (f.match(RX(' "height": \\s*   (\d+)')) ? RegExp.$1 : null);
      if (! (fmt && url2 && !w && !h))
        errorI (id + ": unparsable video formats");
      if (verbose > 2)
        LOG (fmt + ": " + w + "x" + h + ": " + url2);

      var ext = (url2.match(RX('^ [^?&]+ \\. ( [^./?&]+ ) ( [?&] | $ )'))
                 ? RegExp.$1 : null);
      if (!ext) ext = 'mp4';
      var ct = (ext.match(/^(flv|webm|3gpp?)$/s) ? "video/" + ext :
                ext.match(/^(mov)$/s)            ? 'video/quicktime' :
                'video/mpeg');

      var v = { fmt: i,
                url: url2,
                content_type: ct,
                w: w,
                h: h,
             // size: null
             // abr: null
      };
      fmts[i] = v;
      i++;
    }
  }

  return fmts;
}



// Returns:
//  [ title: "T",
//    year: "Y",
//    N: [ ...video info... ],
//    M: [ ...video info... ], ... ]
//
function load_tumblr_formats (id, url, size_p) {

  // The old code doesn't work any more: I guess they locked down the
  // video info URL to require an API key.  So we can just grab the
  // "400" version, I guess...

  var [http, head, body] = get_url (url);
  check_http_status (id, url, http, 1);

  // The following doesn't work any more, but Tumblr is nearly dead anyway.
  // In fact none of this has been tested, because I can't find a single
  // video directly hosted on Tumblr (the Leeroy Jenkins of social media).

  // Incestuous
  if (body.match(RX(' <IFRAME [^<>]*? \\b SRC="   \n\
                   ( https?:// [^<>\"/]*? \\b     \n\
                     ( vimeo\\.com | youtube\\.com |    \n\
                       instagram\\.com )   \n\
                     [^<>\\"]+ )   \n\
                    '))) {
    return load_formats (RegExp.$1, size_p);
  }

  var title = (body.match(/<title>\s*(.*?)<\/title>/si)
               ? RegExp.$1 : null);

  if (! (body.match(RX('<meta \\s+ property="og:type" \\s+   \n\
                        content="[^"<>]*?video@')))) {
    // if (verbose <= 0) exit (1);   // Skip silently if --quiet.
    error ("not a Tumblr video URL: " + url);
  }

  var img = (body.match(RX('<meta \\s+ property="og:image" \\s+   \n\
                           content="([^<>]*?)"@'))
             ? RegExp.$1 : null);
  if (!title) error ("no title: " + url);
  if (!img) error ("no og:image: " + url);
  img = img.replace(/_[^/._]+\.[a-z]+$/si, '_480.mp4');
  if (! img.match(/\.mp4$/s))
    error ("couldn't find video URL: " + url);

  img = img.replace (/^https?:\/\/[^\/]+/si, 'https://vt.tumblr.com');

  title = munge_title (html_unquote (title || ''));
  var fmts = {};
  var i = 0;
  var [w, h] = [0, 0];
  var v = { fmt: i,
            url: img,
            content_type: 'video/mp4',
            w: w,
            h: h,
         // size => null,
         // abr  => null,
  };
  fmts[i] = v;

  fmts['title'] = title;
  fmts['year']  = year;

  return fmts;
}


// Returns:
//  [ title: "T",
//    year: "Y",
//    0: [ ...video info... ],
// Since Instagram only offers one resolution.
//
function load_instagram_formats (id, url, size_p) {
  var [http, head, body] = get_url (url);
  check_http_status (id, url, http, 1);

  var title = (body.match(RX('<meta \\s+ property="og:title" \\s+   \n\
			   content="([^<>]*?)"'))
               ? RegExp.$1 : null);
  var src   = (body.match(RX('<meta \\s+ property="og:video:secure_url" \\s+\n\
			   content="([^<>]*?)"'))
               ? RegExp.$1 : null);
  var w     = (body.match(RX('<meta \\s+ property="og:video:width" \\s+ \n\
			   content="([^<>]*?)"'))
               ? RegExp.$1 : null);
  var h     = (body.match(RX('<meta \\s+ property="og:video:height" \\s+ \n\
			   content="([^<>]*?)"'))
               ? RegExp.$1 : null);
  var ct    = (body.match(RX('<meta \\s+ property="og:video:type" \\s+   \n\
			   content="([^<>]*n?)"'))
               ? RegExp.$1 : null);
  var year  = (body.match (/\bdatetime=\"(\d{4})-/si)
              ? RegExp.$1 : null);

  if (! (src && w && h && ct && title))
      error (id + ": no video in " + url);

  title = munge_title (html_unquote (title || ''));
  ct = ct.replace(/;.*/s, '');

  var fmts = {};

  var i = 0;
  var v = { fmt: i,
            url: src,
            content_type: ct,
            w: w,
            h: h,
         // size: null,
         // abr: null,
  };
  fmts[i]       = v;
  fmts['title'] = title;
  fmts['year']  = year;

  return fmts;
}


// Return the year at which this video was uploaded.
//
var youtube_year_cache = {};
function get_youtube_year (id, body) {

  // Avoid loading the page twice.
  var year = youtube_year_cache[id];
  if (year) return year;

  // 13-May-2015: https://www.youtube.com/watch?v=99lDR6jZ8yE (Lamb)
  // HTML says this:
  //     <strong class="watch-time-text">Uploaded on Oct 28, 2011</strong>
  // But /feeds/api/videos/99lDR6jZ8yE?v=2 says:
  //     <updated>     2015-05-13T21:13:28.000Z
  //     <published>   2015-04-17T15:23:22.000Z
  //     <yt:uploaded> 2015-04-17T15:23:22.000Z
  //
  // And one of my own: https://www.youtube.com/watch?v=HbN4wBJMOuE
  //     <strong class="watch-time-text">Published on Sep 20, 2014</strong>
  //     <published>   2015-04-17T15:23:22.000Z
  //     <updated>     2015-05-16T18:48:26.000Z
  //     <yt:uploaded> 2015-04-17T15:23:22.000Z
  //
  // In fact, I uploaded that on Sep 20, 2014, and when I did I set the
  // Advanced Settings / Recording Date to Sep 14, 2014.  Some time in
  // 2015, I edited the description text.  I have no theory for why the
  // "published" and "updated" dates are different and are both 2015.
  //
  // So, let's scrape the HTML isntead of using the API.
  //
  // (Actually, we don't have a choice now anyway, since they turned off
  // the v2 API in June 2015, and the v3 API requires authentication.)

  // var data_url = ("https://gdata.youtube.com/feeds/api/videos/" + id +
  //                 "?v=2" +
  //                 "&fields=published" +
  //                 "&safeSearch=none" +
  //                 "&strict=true");
  var data_url = "https://www.youtube.com/watch?v=" + id;

  var http;
  var head;
  if (! body) {
    [ http, head, body ] = get_url (data_url);
    if (! check_http_status (id, data_url, http, 0)) return null;
  }

  // var [year, mon, dotm, hh, mm, ss] =
  //   (body.match(/<published>(\d{4})-(\d\d)-(\d\d)T(\d\d):(\d\d):(\d\d)/si));

  var year = (body.match (/\bclass="watch-time-text">[^<>]+\b(\d{4})<\//s)
              ? RegExp.$1 : null);
  youtube_year_cache[id] = year;

  return year;
}


// get_vimeo_year


// Given a list of available underlying videos, pick the ones we want.
//
function pick_download_format (id, site, url, force_fmt, fmts) {

  if (force_fmt === 'all') {
    var all = [];
    for (var k in fmts) {
      if (k !== 'title' && k !== 'year' && k !== 'cipher')
        all.push(k);
    }
    return all.sort();
  }

  if (site === 'vimeo' ||
      site === 'tumblr' ||
      site === 'instagram' ||
      site === 'twitter') {
    // On these sites, just pick the entry with the largest size
    // and/or resolution.

    // No muxing needed on Vimeo
    if (force_fmt === 'mux')
      force_fmt = null;

    if (force_fmt !== null) {
      if (! force_fmt.match(/^\d+$/s))
        error (site + ": fmt must be digits: " + force_fmt);

      for (var k in fmts) {
        if (k === force_fmt) {
          LOG (id + ": forced #" + k + " (" +
               fmts[k]['w'] + " x " +
               fmts[k]['h'] + ")");
          return [k];
        }
      }
      error (id + ": format " + force_fmt + " does not exist");
    }

    var best = null;
    for (var k in fmts) {
      if (k === 'title' || k === 'year' || k === 'cipher')
        continue;
      if (best == null ||
          ((fmts[k]['size'] || 0) > (fmts[best]['size'] || 0) ||
           (fmts[k]['w']    * fmts[k]['h'] >
            fmts[best]['w'] * fmts[best]['h'])))
        best = k;
    }
    LOG (id + ': picked #' + best + " (" +
         fmts[best]['w'] + " x " +
         fmts[best]['h'] + ")");
    return [best];
  } else if (site !== 'youtube') {
    errorI ("unknown site " + site);
  }


  // Divide %known_formats into muxed, video-only and audio-only lists.
  //
  var pref_muxed = [];
  var pref_vo = [];
  var pref_ao = [];

  for (var id2 in known_formats) {
    var fmt = known_formats[id2];
    var v = fmt['v'];
    var a = fmt['a'];
    var b = fmt['abr'];
    var c = fmt['c'];   // channels (e.g. 5.1)
    var w = fmt['w'];
    var h = fmt['h'];
    fmt['desc'] = ((w && h ? w + ' x ' + h + ' ' + v :
                    b ? b + ' kbps ' + a :
                    "?x?") +
                   (c ? ' ' + c : '') +
                   (w && h && b ? '' :
                    w ? ' v/o' : ' a/o'));
    // Ignore 3d video or other weirdo vcodecs.
    if (v && !(v.match(/^(mp4|flv|3gp|webm)$/)))
      continue;

    if (! webm_p) {
      // Skip WebM and Vorbis if desired.
      if (a && !v && a.match(/^(vor)$/))  continue;
      if (!a && v && v.match(/^(webm)$/)) continue;
    }

    if (v && a) {
      pref_muxed.push(id2);
    } else if (v) {
      pref_vo.push(id2);
    } else {
      pref_ao.push(id2);
    }
  }

  // Sort each of those lists in order of download preference.
  //
  for (var S of [pref_muxed, pref_vo, pref_ao]) {
    S.sort(function(ida, idb) {
      var A = known_formats[ida];
      var B = known_formats[idb];

      var aa = A['h'] || 0;			// Prefer taller video.
      var bb = B['h'] || 0;
      if (aa != bb) return (bb - aa);

      aa = ((A['v'] || '') == 'mp4');		// Prefer MP4 over WebM.
      bb = ((B['v'] || '') == 'mp4');
      if (aa != bb) return (bb - aa);

      aa = A['c'] || 0;				// Prefer 5.1 over stereo.
      bb = B['c'] || 0;
      if (aa != bb) return (bb - aa);

      aa = A['abr'] || 0;			// Prefer higher audio rate.
      bb = B['abr'] || 0;
      if (aa != bb) return (bb - aa);

      aa = ((A['a'] || '') == 'aac');		// Prefer AAC over MP3.
      bb = ((B['a'] || '') == 'aac');
      if (aa != bb) return (bb - aa);

      aa = ((A['a'] || '') == 'mp3');		// Prefer MP3 over Vorbis.
      bb = ((B['a'] || '') == 'mp3');
      if (aa != bb) return (bb - aa);

      return 0;
    });
  }


  var mfmt = null;
  var vfmt = null;
  var afmt = null;

  // Find the best pre-muxed format.
  for (var target of pref_muxed) {
    if (fmts[target]) {
      mfmt = target;
      break;
    }
  }

  // If muxing is allowed, find the best un-muxed pair of formats, if
  // such a pair exists that is higher resolution than the best
  // pre-muxed format.
  //
  if (force_fmt === 'mux') {
    for (var target of pref_vo) {
      if (fmts[target]) {
        vfmt = target;
        break;
      }
    }

    // WebM must always be paired with Vorbis audio.
    // MP4 must always be paired with MP3, M4A or AAC audio.
    var want_vorbis_p = (vfmt && known_formats[vfmt]['v'].match(/^webm/si));
    for (var target of pref_ao) {
      if (! fmts[target]) continue;
      var is_vorbis_p = known_formats[target]['a'].match(/^vor/si);
      if (!!want_vorbis_p == !!is_vorbis_p) {
        afmt = target;
        break;
      }
    }

    // If we got one of the formats and not the other, this isn't going to
    // work. Fall back on pre-muxed.
    //
    if ((vfmt || afmt) && !(vfmt && afmt)) {
      LOG ("found " +
           (vfmt ? 'video-only' : 'audio-only') + ' but no ' +
           (afmt ? 'video-only' : 'audio-only') + " formats: ",
           mfmt, vfmt, afmt);
      vfmt = null;
      afmt = null;
    }

    // If the best unmuxed format is not better resolution than the best
    // pre-muxed format, just use the pre-muxed version.
    //
    if (mfmt &&
        vfmt &&
        known_formats[vfmt]['h'] <= known_formats[mfmt]['h']) {
      LOG ("rejecting " + vfmt + " " + afmt + " (" +
           known_formats[vfmt]['w'] + " x " +
           known_formats[vfmt]['h'] + ") for " + mfmt + " (" +
           known_formats[mfmt]['w'] + " x " +
           known_formats[mfmt]['h'] + ")\n");
      vfmt = null;
      afmt = null;
    }
  }

  // If there is a format in the list that we don't know about, warn.
  // This is the only way I have of knowing when new ones turn up...
  //
  {
    var unk = [];
    for (var k in fmts) {
      if (k === 'title' || k === 'year' || k === 'cipher')
        continue;
      if (! known_formats[k]) unk.push(k);
    }
    unk = unk.join(', ', unk.sort());
    if (unk) LOG ("unknown format: " + unk);
  }

  if (verbose > 1) {
    LOG (id + ": available formats:");
    for (var k of Object.keys(fmts).sort()) {
      if (k === 'title' || k === 'year' || k === 'cipher')
        continue;
      LOG (k + " " +
           (known_formats[k]['desc'] || '?') +
           (fmts[k]['dashp']
            ? (' dash' +
               ((typeof (fmts[k]['url']) === 'object')
                ? ' ' + fmts[k]['url'].length + ' segments'
                : ''))
            : ''));
    }
  }

//mfmt=137; //####
  if (vfmt && afmt) {
    if (verbose > 1) {
      var d1 = known_formats[vfmt]['desc'];
      var d2 = known_formats[afmt]['desc'];
      d1 = d1.replace(/ [av]\/?o$/, '');
      d2 = d2.replace(/ [av]\/?o$/, '');
      if (fmts[vfmt]['dashp']) d1 += ' dash';
      if (fmts[afmt]['dashp']) d2 += ' dash';
      LOG (id + ': picked ' + vfmt + ' + ' + afmt +
           ' (' + d1 + ' + ' + d2 + ')');
    }
    return [vfmt, afmt];
  } else if (mfmt) {
    // Either not muxing, or muxing not available/necessary.
    var why = 'picked';
    if (force_fmt !== null && force_fmt !== 'mux') {
      if (! fmts[force_fmt])
        error (id + ': format ' + force_fmt + " does not exist")
      why = 'forced';
      mfmt = force_fmt;
    }
    if (verbose > 1)
      LOG (id + ': ' + why + ' ' + mfmt + " (" +
           (known_formats[mfmt]['desc'] || '???') + ")");
    return [ mfmt ];
  } else {
    error (id + ": No pre-muxed formats; \"ffmpeg\" required for download");
  }
}


// Returns the title and URLs of every video in the playlist.
//
function youtube_playlist_urls(id, url) {

  var playlist = [];
  var start = 0;

  var [ http, head, body ] = get_url (url);
  check_http_status (id, url, http, 1);

  var title = '';
  if (body.match(/"title":"(.*?)","description"/s))
    title = RegExp.$1;

  title = munge_title (title);
  if (!title) title = 'Untitled Playlist';

  // This is very different than the corresponding Perl function because
  // we're getting very different document bodies, presumably based upon
  // the user agent. Hooray.
  //
  // I imagine I could sync them if I updated the user-agent in the Perl
  // code to be more current, and I will probably have to do that eventually,
  // but that might well unleash other horrors that I'd rather put off until
  // absolutely necessary.

  if (body.match(/window\[.ytInitialData.\] *= *(.*?);/s))
    body = RegExp.$1;
  else
    error ("playlist html unparsable", url);

  // Get the up-to-100 videos that came with the document.
  //
  body = body.replace(/\n/g, ' ');
  body = body.replace(/\\/g, '');
  body = body.replace (/("playlistVideoRenderer")/g, "\n$1");
  var prev_id = null;
  for (var chunk of body.split(/\n/)) {
    var id;
    if (chunk.match(/\{"videoId":"(.*?)"/si))
      id = RegExp.$1;
    var t2;
    if (chunk.match(/"simpleText":"(.*?)"/si))
      t2 = RegExp.$1;
    if (id == prev_id) continue;
    prev_id = id;
    if (id && t2) {
      t2 = munge_title (html_unquote (t2));
      playlist.push ({ title: t2,
                       url: 'https://www.youtube.com/watch?v=' + id });
    }
  }

  if (!playlist.length) errorI (id + ': no playlist entries');

  // Scraping the HTML only gives us the first hundred videos if the
  // playlist has more than that. To get the rest requires more work.
  //
  var more = null;
  if (body.match(
       /"continuation"\s*:\s*"(.+?)".*?"clickTrackingParams"\s*:\s*"(.+?)"/si))
    more = ('/browse_ajax' +
            '?ctoken=' + RegExp.$1 +
            '&amp;continuation=' + RegExp.$1 +
            '&amp;itct=' + RegExp.$2);

 var vv = null;
 if (body.match(/"client.version","value":"(.*?)"/si))
   vv = RegExp.$1;

  var page = 2;
  while (more) {
    more = html_unquote (more);
    if (more.match(/^\/\//s)) more = 'https:' + more;
    if (more.match(/^\//s))   more = 'https://www.youtube.com' + more ;
    if (verbose > 1) LOG('loading playlist page ' + page);
    [ http, head, body ] = get_url_hdrs (more,
                                         // You absolute bastards!!
                                         { 'X-YouTube-Client-Name': 1,
                                           'X-YouTube-Client-Version': vv });
    check_http_status (id, more, http, 1);

    // Get the next up-to-100 videos.
    //
    body = body.replace(/\n/g, ' ');
    body = body.replace(/\\/g, '');
    body = body.replace (/("playlistVideoRenderer")/g, "\n$1");
    for (var chunk of body.split(/\n/)) {
      var id;
      if (chunk.match(/\{"videoId":"(.*?)"/si))
        id = RegExp.$1;
      var t2;
      if (chunk.match(/"simpleText":"(.*?)"/si))
        t2 = RegExp.$1;
      if (id == prev_id) continue;
      prev_id = id;
      if (id && t2) {
        t2 = munge_title (html_unquote (t2));
        playlist.push ({ title: t2,
                         url: 'https://www.youtube.com/watch?v=' + id });
      }
    }

    more = null;
    if (body.match(
       /"continuation"\s*:\s*"(.+?)".*?"clickTrackingParams"\s*:\s*"(.+?)"/si))
      more = ('/browse_ajax' +
              '?ctoken=' + RegExp.$1 +
              '&amp;continuation=' + RegExp.$1 +
              '&amp;itct=' + RegExp.$2);
    page++;
  }

  // Prefix each video's title with the playlist's title and its index.
  //
  var i = 0;
  var count = playlist.length;
  for (var P of playlist) {
    i++;
    var t2 = P.title;
    t2 = munge_title (html_unquote (t2));
    var ii = ("0000" + i).slice (count > 999 ? -3 :
                                 count >  99 ? -3 : -2);
    P.title = title + ": " + ii + ": " + t2;
  }

  return { title: title, urls: playlist };
}


// I think I'll omit this from the JS port.
// The Perl version of this function is fairly music video idiosyncratic.
//
function munge_title (title) {
  // At least omit the branding.
  title = title.replace(/^Youtube *$/si, '');
  title = title.replace(/^Youtube -+ /si, '');
  title = title.replace(/ -+ Youtube$/si, '');
  title = title.replace(/ on Vimeo\s*$/si, '');
  title = title.replace(/Broadcast Yourself\.?$/si, '');
  return title || 'Untitled';
}


// There are so many ways to specify URLs of videos... Turn them all into
// something sane and parsable.
//
// Duplicated in youtubefeed.
//
function canonical_url (url) {

  // Forgive pinheaddery.
  url = url.replace(/&amp;/gs, '&');
  url = url.replace(/&amp;/gs, '&');

  // Add missing "https:"
  if (! url.match(/^https?:\/\//si))
    url = "https://" + url;

  // Rewrite youtu.be URL shortener.
  url = url.replace(/^https?:\/\/([a-z]+\.)?youtu\.be\//si,
                    'https://youtube.com/v/');

  // Youtube's "attribution links" don't encode the second URL:
  // there are two question marks. FFS.
  // https://www.youtube.com/attribution_link?u=/watch?v=...&feature=...
  url = url.replace(
            /^(https?:\/\/[^\/]*\byoutube\.com\/)attribution_link\?u=\//gsi,
            '$1');

  // Rewrite Vimeo URLs so that we get a page with the proper video title:
  // "/...#NNNNN" => "/NNNNN"
  url = url.replace(/^(https?:\/\/([a-z]+\.)?vimeo\.com\/)[^\d].*\#(\d+)$/s,
                    '$1$3');

  url = url.replace(/^http:/s, 'https:');	// Always https.

  var id = null;
  var site = null;
  var playlist_p = false;

  // Youtube /view_play_list?p= or /p/ URLs.
  if (url.match(RX('^https?://(?:[a-z]+\\.)?(youtube) (?:-nocookie)? \\.com/\n\
                (?: view_play_list\\?p= |              \n\
                    p/ |                               \n\
                    embed/p/ |                         \n\
                    .*? [?&] list=(?:PL)? |            \n\
                    embed/videoseries\\?list=(?:PL)?   \n\
                )                                      \n\
                ([^<>?&,]+) ($|&)'))) {
    site = RegExp.$1;
    id   = RegExp.$2;
    url  = "https://www." + site + ".com/view_play_list?p=" + id;
    playlist_p = true;

  // Youtube "/verify_age" URLs.
  } else if (url.match(RX('\n\
             ^https?://(?:[a-z]+\\.)?(youtube) (?:-nocookie)? \\.com/+   \n\
	     .* next_url=([^&]+)')) ||
             url.match(RX('^https?://(?:[a-z]+\\.)?google\\.com/   \n\
                     .* service = (youtube)                      \n\
                     .* continue = ( http%3A [^?&]+)')) ||
             url.match(RX('^https?://(?:[a-z]+\\.)?google\\.com/   \n\
                     .* service = (youtube)                      \n\
                     .* next = ( [^?&]+)'))
           ) {
    site = RegExp.$1;
    url = decodeURIComponent(RegExp.$2);
    if (url.match(/&next=([^&]+)/s)) {
      url = decodeURIComponent(RegExp.$1);
      url = url.replace(/&.*$/s, '');
    }
    if (url.match(/^\//s))
      url = "https://www." + site + ".com" + url;
    return canonical_url(url);

  // Youtube /watch/?v= or /watch#!v= or /v/ URLs.
  } else if (url.match(RX('^https?:// (?:[a-z]+\\.)?   \n\
                     (youtube) (?:-nocookie)? (?:\\.googleapis)? \\.com/+   \n\
                     (?: (?: watch/? )? (?: \\? | \\#! ) v= |               \n\
                         v/ |                                               \n\
                         embed/ |                                           \n\
                         .*? &v= |                                          \n\
                         [^/\\#?&]+ \\#p(?: /[a-zA-Z\\d] )* /               \n\
                     )                                                      \n\
                     ([^<>?&,\'\"]+) ($|[?&])'))) {
    site = RegExp.$1;
    id   = RegExp.$2;
    url = "https://www." + site + ".com/watch?v=" + id;

  // Youtube "/user" and "/profile" URLs.
  } else if (url.match(RX('\n\
              ^https?://(?:[a-z]+\\.)?(youtube) (?:-nocookie)? \\.com/   \n \
                     (?:user|profile).*\\#.*/([^&/]+)'))) {
    site = RegExp.$1;
    id   = decodeURIComponent(RegExp.$2);
    url  = "https://www." + site + ".com/watch?v=" + id;
    if (!id) error ("unparsable user next_url: " + url);

  // Vimeo /NNNNNN URLs
  // and player.vimeo.com/video/NNNNNN
  // and vimeo.com/m/NNNNNN
  } else if (url.match(
          /^https?:\/\/(?:[a-z]+\.)?(vimeo)\.com\/(?:video\/|m\/)?(\d+)/s)) {
    site = RegExp.$1;
    id   = RegExp.$2;
    url = "https://" + site + ".com/" + id;

  // Vimeo /videos/NNNNNN URLs.
  } else if (url.match(
              /^https?:\/\/(?:[a-z]+\.)?(vimeo)\.com\/.*\/videos\/(\d+)/s)) {
    site = RegExp.$1;
    id   = RegExp.$2;
    url  = "https://" + site + ".com/" + id;

  // Vimeo /channels/name/NNNNNN URLs.
  // Vimeo /ondemand/name/NNNNNN URLs.
  } else if (url.match(
          /^https?:\/\/(?:[a-z]+\.)?(vimeo)\.com\/[^\/]+\/[^\/]+\/(\d+)/s)) {
    site = RegExp.$1;
    id   = RegExp.$2;
    url  = "https://" + site + ".com/" + id;

  // Vimeo /album/NNNNNN/video/MMMMMM
  } else if (url.match(
       /^https?:\/\/(?:[a-z]+\.)?(vimeo)\.com\/album\/\d+\/video\/(\d+)/s)) {
    site = RegExp.$1;
    id   = RegExp.$2;
    url = "https://" + site + ".com/" + id;

  // Vimeo /moogaloop.swf?clip_id=NNNNN
  } else if (url.match(
                /^https?:\/\/(?:[a-z]+\.)?(vimeo)\.com\/.*clip_id=(\d+)/s)) {
    site = RegExp.$1;
    id   = RegExp.$2;
    url  = "https://" + site + ".com/" + id;

  // Tumblr /video/UUU/NNNNN
  } else if (url.match(
   /^https?:\/\/[-_a-z\d]+\.(tumblr)\.com\/video\/([^\/]+)\/(\d{8,})\//si)) {
    var user;
    site = RegExp.$1.toLowerCase();
    user = RegExp.$2;
    id   = RegExp.$3;
    url  = "https://" + user + "." + site + ".com/post/" + id;

  // Tumblr /post/NNNNN
  } else if (url.match(RX('^https?://([-_a-z\\d]+)\\.(tumblr)\\.com   \n\
                     /.*?/(\\d{8,})(/|$)'))) {
    var user;
    user = RegExp.$1;
    site = RegExp.$2;
    id   = RegExp.$3;
    url  = "https://" + user + "." + site + ".com/post/" + id;

  // Instagram /p/NNNNN
  } else if (url.match(
           /^https?:\/\/([-_a-z\d]+\.)?(instagram)\.com\/p\/([^\/?&]+)/si)) {
    site = RegExp.$2.toLowerCase();
    id   = RegExp.$3;
    url  = "https://www." + site + ".com/p/" + id;

  // Twitter /USER/status/NNNNN
  } else if (url.match(RX('\n\
            ^https?://([-_a-z\\d]+\\.)?(twitter)\\.com/([^/?&]+)   \n\
                     /status/([^/?&]+)'))) {
    var user;
    site = RegExp.$2.toLowerCase();
    user = RegExp.$3;
    id   = RegExp.$4;
    url  = "https://" + site + ".com/" + user + "/status/" + id;

  } else {
    error ("unparsable URL: " + url);
  }

  if (id && id.match(/[\/:?]/s)) error ("bogus URL: " + url);

  return [ url, id, site ];
}


function canonical_url_selftest() {
  for (var url of
         ['https://www.youtube.com/view_play_list?p=000&playnext=1',
          'https://youtube.com/verify_age/XXX/next_url=https%3A//youtube.com/watch/%3Fv=%30%30%30&yyy=XXX',
          'https://www.youtube.com/watch?v=000',
          'https://www.youtube.com/watch?v=000&yyy=XXX',
          'https://www.youtube.com/watch#!v=000',
          'https://www.youtube.com/profile/XXX/#YYY/%30%30%30',
          'https://vimeo.com/000',
          'https://player.vimeo.com/video/000',
          'https://vimeo.com/m/000',
          'https://vimeo.com/XXX/videos/000',
          'https://vimeo.com/channel/XXX/000',
          'https://vimeo.com/album/999/video/000',
          'https://vimeo.com/mogaloop.swf?clip_id=000',
          'https://www.tumblr.com/video/XXX/00000000',
          'https://www.tumblr.com/post/00000000',
          'https://www.instagram.com/p/000',
          'https://twitter.com/XXX/status/000',
          'https://www.youtube.com/p/000',

          'youtu.be/000',
          'https://www.youtube.com/attribution_link?u=/watch?v=000&feature=Y',
          'youtube.com/v/000',
          'https://vimeo.com/XXX\#000',
          ]) {
    var a = canonical_url(url);
    if (a[1] && a[1].match(/^000+$/s))
      LOG("OK", url, a);
    else
      LOG("FAIL", url, a);
  }
}
//canonical_url_selftest();


function content_type_ext (ct) {
  if      (ct.match(/\/(x-)?flv$/si))  { return 'flv';  }
  else if (ct.match(/\/(x-)?webm$/si)) { return 'webm'; }
  else if (ct.match(/\/(x-)?3gpp$/si)) { return '3gpp'; }
  else if (ct.match(/\/quicktime$/si)) { return 'mov';  }
  else if (ct.match(/^audio\/mp4$/si)) { return 'm4a';  }
  else                                 { return 'mp4';  }
}


function load_formats (url, size_p) {
  var [url, id, site] = canonical_url(url);
  if (site == 'youtube')   return load_youtube_formats   (id, url, size_p);
  if (site == 'vimeo')     return load_vimeo_formats     (id, url, size_p);
  if (site == 'tumblr')    return load_tumblr_formats    (id, url, size_p);
  if (site == 'instagram') return load_instagram_formats (id, url, size_p);
  error (id + ": unknown site: " + site);
}


function download_video_url (url, size_p, force_fmt) {
  var [url, id, site] = canonical_url(url);

  var ret = [];

  // Fuck you, Twitter. Handle links to Youtube inside twits.
  // If there is both a Youtube link and Twitter-hosted video,
  // we ignore the latter.
  //
  if (site == 'twitter') {
    var [http, head, body] = get_url (url);
    check_http_status (id, url, http, 1);
    if (body.match(
     /\b(https?:\/\/(youtu\.be|[^a-z\/]+\.youtube\.com)\/[^\s\"\'<>]+)/si)) {
      [ url, id, site ] = canonical_url (RegExp.$1);
    }
  }

  // Though Tumblr and Twitter can host their own videos, much of the time
  // there is just an embedded Youtube video instead.
  //
  if (site == 'tumblr' || site == 'twitter') {
    [ http, head, body ] = get_url (url);
    check_http_status (id, url, http, 1);
    if (body.match(RX(' \\b ( https?:// (?: [a-z]+\\. )?   \n\
                         youtube\\.com/                    \n\
                         [^\"\'<>]*? (?: embed | \\?v= )   \n\
                         [^\"\'<>]+ )'))) {
      [ url, id, site ] = canonical_url (html_unquote (RegExp.$1));
    }
  }

  // Videos can come in multiple resolutions, and sometimes with audio and
  // video in separate URLs. Get the list of all possible downloadable video
  // formats.
  //
  var fmts = load_formats (url, size_p);

  var title = munge_title (fmts['title']);

  // Now that we have the video info, decide what to download.
  // If we're doing --fmt all, this is all of them.
  // Otherwise, it's either one URL or two (audio + video mux).
  //
  var targets = (fmts
                 ? pick_download_format (id, site, url, force_fmt, fmts)
                 : []);
  var pair = (targets.length == 2 && force_fmt != 'all' ? targets : null);

  if (size_p && pair) {
    // With --size, we only need to examine the first pair of the mux.
    if (pair[0]) targets = (pair[0]);
    pair = null;
  }

  // outfiles stuff

  for (var target of targets) {
    var fmt   = fmts[target];
    var ct    = fmt['content_type'];
    var w     = fmt['width'];
    var h     = fmt['height'];
    var abr   = fmt['abr'];
    var size  = fmt['size'];
    var url2  = fmt['url'];
    var dashp = fmt['dashp'];

    if (size_p) {
      if (! ((w && h) || abr)) {
        var [w2, h2, s2, a2] =
          video_url_size (id, url2, ct,
                          (force_fmt === 'all' ? true : false));
        if (w2) w    = w2;
        if (h2) h    = h2;
        if (s2) size = s2;
        if (a2) abr  = a2;
      }

      var ii = id + (targets.size == 1 ? '' : ":" + target);
      var ss = fmt_size (size);
      var wh = (w && h
                ? w + " x " + h
                : (abr ? abr + "  " : ' ?x?'));
      return ii + "\t" + wh + "\t" + ss + "\t" + t2;

    } else {

      var prefix = '';
      var append_suffix_p = true; // ####

      var suf = (append_suffix_p
                 ? (" [" + id +
                    ((targets.length == 1 &&
                      !(force_fmt == 'all'))
                     ? '' : " " + target) +
                    "]")
                 : (pair
                    ? (target == pair[0] ? '.video-only' : '.audio-only')
                    : ''));
      var file = (prefix ? prefix + " " + title : title) + suf;
      ct = ct.replace(/;.*$/, '');
      file += '.' + content_type_ext (ct);

      fmt['file'] = file;

      ret.push (fmt);
    }
  }

  return ret;
}


// Returns a list of the underlying audio/video URLs to download,
// Note: These URLs will expire and stop working in minutes-to-hours.
// Results will look like one of the following:
//
//   - A single URL:
//
//       [ { url: "https...",
//           w: WIDTH, h: HEIGHT, abr: AUDIO_BITRATE, size: BYTES,
//           file: "recommended download file name.mp4" } ]
//
//   - A single multi-segment URL, where each segment must be concatenated:
//
//       [ { url: [ "https:1", "https:2", "https:3", "https:4", ... ],
//           w: WIDTH, ... } ]
//
//   - A pair of URLs, one audio-only and one video-only, which must be muxed
//     together (e.g., with ffmpeg) before they will be usable by most players:
//
//       [ { url: "https:V/O", w: WIDTH, h: HEIGHT, ... },
//         { url: "https:A/O", abr: AUDIO_BITRATE,  ... } ]
//
//   - A pair of audio and video URLs that are also segmented:
//
//       [ { url: [ "https:v1", "https:v2", "https:v3", ... ], ... }
//         { url: [ "https:a1", "https:a2", "https:a3", ... ], ... } ]
//
// By default it picks the "best" URL, but 'force_fmt' can be used to
// override it with a specific low-level format number.
//
function youtubedown_urls (url, force_fmt) {
  if (force_fmt === undefined || force_fmt === null || force_fmt === '')
    force_fmt = 'mux';
  return download_video_url (url, false, force_fmt);
}

function youtubedown_playlist_urls(url) {
  var [ url2, id, site ] = canonical_url (url);
  return youtube_playlist_urls (id, url2);
}


// Download the best video or videos to the Downloads folder.
// If output_node is specified, log diagnostics into that DIV.
//
function youtubedown (url, output_node, force_fmt) {
  log_node = output_node;

  var go = function() {
    var fmts = youtubedown_urls (url, force_fmt);
    for (var fmt of fmts) {
      if (verbose)
        LOG ('downloading ' +
             (fmt.w    ? fmt.w + 'x' + fmt.h + ' ' : '') +
             (fmt.abr  ? fmt.abr + 'kbps '         : '') +
             (fmt.size ? fmt_size (fmt.size) + ' ' : '') +
             fmt.file);
      download_video_file (fmt.file, fmt.url);
    }
  };

  if (!log_node) {
    go();  // don't blow away the stack trace if we aren't DIV-logging.
  } else {
    try {
      go();
    } catch (e) {
      LOG ("error: " +
           (e.line ? e.line + ": " : "") +
           e.toString());
    }
  }
}


// Export:
//
window.youtubedown_version       = version;
window.youtubedown_urls          = youtubedown_urls;
window.youtubedown_playlist_urls = youtubedown_playlist_urls;
window.youtubedown               = youtubedown;


// Test cases:
// 18 137+140 https://www.youtube.com/watch?v=sYsXKhBknyM Eve of Destruction
// 22 137+140-dash https://www.youtube.com/watch?v=I_MkW0CW4QM VFeedback


/*
;; Yo dawg, we heard you liked regexps, so we put regexps in your regexps
;; so you can regexp while you sexpr.

(defun ytre()
  (query-replace-regexp
   (concat "\\$\\([_a-z\d]+\\)"
           "[ \t\n]*=~[ \t\n]*"
           "s\\(.\\)\\(.*?\\)\\2\\(.*?\\)\\2\\([a-z]*\\);")
   "\\1 = \\1.replace(/\\3/\\5, '\\4');"))

(defun ytre2()
  (query-replace-regexp
   (concat "\\$\\([_a-z\d]+\\)"
           "[ \t\n]*=~[ \t\n]*"
           "m\\(.\\)\\(.*?\\)\\2\\([a-z]*\\)")
   "\\1.match(/\\3/\\4)"))

(defun ytrx()
  (let (p0 p1 s)
    (re-search-forward "=~[ \t\n]*m\\(.\\)")
    (setq p0 (point-marker))
    (setq s (match-string 1))
    (delete-region (match-beginning 0) (match-end 1))
    (insert ".match(RX('")
    (re-search-forward (concat s "[a-wz]*x"))
    (setq p1 (point-marker))
    (save-restriction
      (narrow-to-region p0 p1)
      (goto-char p0)
      (while (search-forward "\\" nil t)
        (insert "\\"))
      (goto-char p0)
      (while (search-forward "\n" nil t)
        (forward-char -1)
        (insert "   \\n\\")
        (forward-char 1))
      (goto-char p1)
      (insert "'))")
      )))

*/

})();
