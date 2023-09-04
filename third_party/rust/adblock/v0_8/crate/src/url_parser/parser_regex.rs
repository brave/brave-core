use regex::Regex;

pub fn get_hostname_regex(url: &str) -> Option<(usize, (usize, usize))> {
    lazy_static! {
        static ref HOSTNAME_REGEX_STR: &'static str = concat!(
            r"(?P<scheme>[a-z][a-z0-9+\-.]*)://",               // Scheme
            r"(?:[a-z0-9\-._~%!$&'()*+,;=]+@)?",                // User
            r"(?P<host>[\w\-.~%]+",                             // Named host
            r"|\[[a-f0-9:.]+\]",                                // IPv6 host
            r"|\[v[a-f0-9][a-z0-9\-._~%!$&'()*+,;=:]+\])",      // IPvFuture host
            // r"(?::[0-9]+)?",                                  // Port
            // r"(?:/[a-z0-9\-._~%!$&'()*+,;=:@]+)*/?",          // Path
            // r"(?:\?[a-z0-9\-._~%!$&'()*+,;=:@/?]*)?",         // Query
            // r"(?:\#[a-z0-9\-._~%!$&'()*+,;=:@/?]*)?",         // Fragment
        );
        static ref HOST_REGEX: Regex = Regex::new(&HOSTNAME_REGEX_STR).unwrap();
    }

    HOST_REGEX.captures(url)
        .and_then(|c| {
            Some((c.name("scheme")?.end(), (c.name("host")?.start(), c.name("host")?.end())))
        })
}

pub fn get_url_host(url: &str) -> Option<(String, usize, (usize, usize))> {
    let decode_flags = idna::uts46::Flags {
        use_std3_ascii_rules: true,
        transitional_processing: true,
        verify_dns_length: true,
    };
    get_hostname_regex(&url)
        .and_then(|(schema_end, (hostname_start, hostname_end))| {
            let host = &url[hostname_start..hostname_end];
            if host.is_ascii() {
                Some((url.to_owned(), schema_end, (hostname_start, hostname_end)))
            } else {
                idna::uts46::to_ascii(&host, decode_flags).map(|h| {
                    let normalised = format!("{}://{}{}", &url[..schema_end], &h, &url[hostname_end..]);
                    (normalised, schema_end, (hostname_start, hostname_start + h.len()))
                }).ok()
            }
        })
}

impl super::UrlParser for crate::request::Request {
    fn parse_url(url: &str) -> Option<super::RequestUrl> {
        let parsed = get_url_host(&url);
        parsed.map(|(url, schema_end, (host_start, host_end))| {
            super::RequestUrl {
                url: url,
                schema_end: schema_end,
                hostname_pos: (host_start, host_end),
                domain: super::get_host_domain(&url[host_start..host_end])
            }
        })
    }
}


#[cfg(test)]
mod parse_tests {
    use super::*;

    #[test]
    // pattern
    fn parses_hostname() {
        assert_eq!(get_url_host("http://example.foo.edu.au"), Some(("http://example.foo.edu.au".to_owned(), 4, (7, 25))));
        assert_eq!(get_url_host("http://example.foo.edu.sh"), Some(("http://example.foo.edu.sh".to_owned(), 4, (7, 25))));
        assert_eq!(get_url_host("http://example.foo.nom.br"), Some(("http://example.foo.nom.br".to_owned(), 4, (7, 25))));
        assert_eq!(get_url_host("http://example.foo.nom.br:80/"), Some(("http://example.foo.nom.br:80/".to_owned(), 4, (7, 25))));
        assert_eq!(get_url_host("http://example.foo.nom.br:8080/hello?world=true"), Some(("http://example.foo.nom.br:8080/hello?world=true".to_owned(), 4, (7, 25))));
        assert_eq!(get_url_host("http://example.foo.nom.br/hello#world"), Some(("http://example.foo.nom.br/hello#world".to_owned(), 4, (7, 25))));
        assert_eq!(get_url_host("http://127.0.0.1:80"), Some(("http://127.0.0.1:80".to_owned(), 4, (7, 16))));
        assert_eq!(get_url_host("http://[2001:470:20::2]"), Some(("http://[2001:470:20::2]".to_owned(), 4, (7, 23))));
        assert_eq!(get_url_host("http://[2001:4860:4860::1:8888]"), Some(("http://[2001:4860:4860::1:8888]".to_owned(), 4, (7, 31))));
    }
}
