use serde::{Deserialize, Serialize};
use std::collections::HashMap;

use crate::speedreader::{AttributeRewrite, RewriteRules, SpeedReaderConfig, SpeedReaderError};

const IMAGE_TARGET_WIDTH: u32 = 600;

#[derive(Serialize, Deserialize)]
pub struct Whitelist {
    map: HashMap<String, SpeedReaderConfig>,
}

impl Default for Whitelist {
    fn default() -> Self {
        Whitelist {
            map: HashMap::new(),
        }
    }
}

impl Whitelist {
    pub fn add_configuration(&mut self, config: SpeedReaderConfig) {
        self.map.insert(config.domain.clone(), config);
    }

    pub fn get_configuration(&self, domain: &str) -> Option<&SpeedReaderConfig> {
        if let Some(config) = self.map.get(domain) {
            return Some(config);
        }

        for (i, c) in domain[..domain.len() - 2].char_indices() {
            if c == '.' {
                let subdomain = &domain[i + 1..];
                let maybe_config = self.map.get(subdomain);
                if maybe_config.is_some() {
                    return maybe_config;
                }
            }
        }

        None
    }

    pub fn get_url_rules(&self) -> Vec<String> {
        self.map
            .values()
            .flat_map(|c| c.url_rules.iter().cloned())
            .collect()
    }

    pub fn serialize(&self) -> Result<Vec<u8>, SpeedReaderError> {
        let mut out = Vec::new();
        rmps::encode::write(&mut out, &self)?;
        Ok(out)
    }

    pub fn deserialize(serialized: &[u8]) -> Result<Self, SpeedReaderError> {
        Ok(rmps::decode::from_read(serialized)?)
    }

    pub fn load_predefined(&mut self) {
        self.add_configuration(SpeedReaderConfig {
            domain: "cnet.com".to_owned(),
            url_rules: vec![
                "||cnet.com/features/*".to_owned(),
                "||cnet.com/roadshow/reviews/*".to_owned(),
                "||cnet.com/roadshow/news/*".to_owned(),
                "||cnet.com/news/*".to_owned(),
                "||cnet.com/reviews/*".to_owned(),
            ],
            declarative_rewrite: Some(RewriteRules {
                main_content: vec![".article-main-body".to_owned(), ".hero-content".to_owned()],
                main_content_cleanup: vec![
                    "footer".to_owned(),
                    "noscript".to_owned(),
                    ".c-head_bottomWrapper".to_owned(),
                    ".c-head_share".to_owned(),
                    ".social-button-small-author".to_owned(),
                    ".clickToEnlarge".to_owned(),
                    ".gallery".to_owned(),
                    ".video".to_owned(),
                    ".svg-symbol".to_owned(),
                ],
                delazify: true,
                fix_embeds: true,
                ..RewriteRules::default()
            }),
        });

        self.add_configuration(SpeedReaderConfig {
            domain: "247sports.com".to_owned(),
            url_rules: vec!["||247sports.com/Article/".to_owned()],
            declarative_rewrite: Some(RewriteRules {
                main_content: vec!["section .article-cnt".to_owned()],
                main_content_cleanup: vec![".article-cnt__header > .container".to_owned()],
                fix_embeds: true,
                ..RewriteRules::default()
            }),
        });

        self.add_configuration(SpeedReaderConfig {
            domain: "abcnews.go.com".to_owned(),
            url_rules: vec![
              "||abcnews.go.com/*/story".to_owned(),
              "||abcnews.go.com/*/wireStory".to_owned(),
            ],
            declarative_rewrite: Some(RewriteRules {
            main_content: vec![".Article__Wrapper".to_owned(), "body > script:not([src])".to_owned()],
            main_content_cleanup: vec![
                ".CalloutLink".to_owned(), ".Article__Footer".to_owned(), ".Article__Header .Share".to_owned(),
                ".MediaPlaceholder__Overlay".to_owned(),
                ".inlineElement > iframe".to_owned(),
                ".Screen__Reader__Text".to_owned(), ".taboola".to_owned(),
            ],
            fix_embeds: true,
            content_script: Some(r#"<script>
            document.querySelector(".FeaturedMedia figure img").src =
                JSON.parse(document.querySelector('script[type="application/ld+json"]').innerText).image.url;
            [...document.querySelectorAll(".InlineImage .Image__Wrapper img")]
                .map((e, i) => e.src =
                    __abcnews__.page.content.story.everscroll[0].inlines.filter(d => d.type === "image").map(i => i.imageSrc)[i])
            </script>"#.to_owned()),
            ..RewriteRules::default()
        })});

        self.add_configuration(SpeedReaderConfig {
            domain: "cnn.com".to_owned(),
            url_rules: vec![
                r#"/cnn.com\/(\d){4}\/(\d){2}\/(\d){2}\/.*index.html/"#.to_owned(),
                r#"||cnn.com/*/article/*/index.html"#.to_owned(),
            ],
            declarative_rewrite: Some(RewriteRules {
                main_content: vec![
                    ".pg-headline".to_owned(),
                    ".metadata".to_owned(),
                    ".media__video--thumbnail-wrapper img".to_owned(),
                    "[itemprop=\"articleBody\"]".to_owned(),
                ],
                main_content_cleanup: vec![
                    ".m-share".to_owned(),
                    ".pg-comments".to_owned(),
                    "[class*=\"outbrain\"]".to_owned(),
                    ".zn-story-bottom".to_owned(),
                    ".zn-body__read-more".to_owned(),
                ],
                fix_embeds: true,
                ..RewriteRules::default()
            }),
        });

        self.add_configuration(SpeedReaderConfig {
            domain: "nytimes.com".to_owned(),
            url_rules: vec![
                r#"/www.nytimes.com\/\d{4}\/\d{2}\/\d{2}\/([^\/]+(\/)?){2,3}\.html/"#.to_owned(),
            ],
            declarative_rewrite: Some(RewriteRules {
                main_content: vec![
                    "div.g-blocks".to_owned(),
                    "section[name=\"articleBody\"]".to_owned(),
                    "article header".to_owned(),
                ],
                main_content_cleanup: vec![
                    ".ad".to_owned(),
                    "header#story-header".to_owned(),
                    ".story-body-1 .lede.video".to_owned(),
                    ".visually-hidden".to_owned(),
                    "#newsletter-promo".to_owned(),
                    ".promo".to_owned(),
                    ".comments-button".to_owned(),
                    ".hidden".to_owned(),
                    ".comments".to_owned(),
                    ".supplemental".to_owned(),
                    ".nocontent".to_owned(),
                    ".story-footer-links".to_owned(),
                    "#sponsor-wrapper".to_owned(),
                    "[role=\"toolbar\"]".to_owned(),
                    "header > section".to_owned(),
                ],
                fix_embeds: true,
                content_script: Some(
                    r#"
        <script>
        [...document.querySelectorAll("figure[itemid]")].forEach(fig => {
            let lazy = fig.querySelector("[data-testid=\"lazyimage-container\"]");
            if (lazy) { lazy.innerHTML = "<img src='" + fig.getAttribute("itemid") + "'>" }
        });
        </script>
        "#
                    .to_owned(),
                ),
                ..RewriteRules::default()
            }),
        });

        self.add_configuration(SpeedReaderConfig {
            domain: "theguardian.com".to_owned(),
            url_rules: vec![
                r#"/theguardian.com\/.*\/(\d){4}\/\w+\/(\d){2}\/.*/"#.to_owned()
            ],
            declarative_rewrite: Some(RewriteRules {
            main_content: vec![
                "article header".to_owned(), ".content__article-body".to_owned(),
            ],
            main_content_cleanup: vec![
                ".hide-on-mobile".to_owned(), ".inline-icon".to_owned(),
                ".atom__button".to_owned(), "input".to_owned(),
                ".meta__extras".to_owned(), ".content__headline-showcase-wrapper".to_owned(),
                ".fc-container__header".to_owned(),
                "figure.element-embed".to_owned(),
                ".vjs-control-text".to_owned(),
            ],
            delazify: true,
            fix_embeds: true,
            content_script: Some(r#"<script>
            [...document.querySelectorAll("[data-src-background]")]
                .map(d => d.src = d.dataset["src-background"].replace("background-image: url", "").replace(/[\(\)]/g, ""))
            </script>"#.to_owned()),
            preprocess: vec![
                AttributeRewrite {
                    selector: ".vjs-big-play-button[style]".to_owned(),
                    attribute: Some(("style".to_owned(), "data-src-background".to_owned())),
                    element_name: "img".to_owned()
                }
            ],
        })});

        self.add_configuration(SpeedReaderConfig {
            domain: "washingtonpost.com".to_owned(),
            url_rules: vec![
                r#"/washingtonpost.com\/.*\/(\d){4}\/(\d){2}\/(\d){2}\/\w+/"#.to_owned(),
                r#"||washingtonpost.com*_story.html"#.to_owned(),
                r#"! travel pages currently handled poorly"#.to_owned(),
                r#"@@||washingtonpost.com/travel"#.to_owned(),
                // r#"||thelily.com/*/"#.to_owned(),
            ],
            declarative_rewrite: Some(RewriteRules {
                main_content: vec![
                    "main > header".to_owned(),
                    "main > article .byline".to_owned(),
                    "main > article [data-qa=\"timestamp\"]".to_owned(),
                    "main > article figure".to_owned(),
                    ".article-body".to_owned(),
                    ".ent-article-body".to_owned(),
                    "[data-feature-name^=\"etv3\"]".to_owned(),
                ],
                main_content_cleanup: vec![
                    "header > nav".to_owned(),
                    ".tooltip".to_owned(),
                    "[data-qa=\"article-body-ad\"]".to_owned(),
                ],
                preprocess: vec![AttributeRewrite {
                    selector: "[data-fallback-image-url]".to_owned(),
                    attribute: Some(("data-fallback-image-url".to_owned(), "src".to_owned())),
                    element_name: "img".to_owned(),
                }],
                ..RewriteRules::default()
            }),
        });

        self.add_configuration(SpeedReaderConfig {
            domain: "foxnews.com".to_owned(),
            url_rules: vec![
                r#"@@||video.foxnews.com"#.to_owned(),
                // r#"||foxbusiness.com/business-leaders/*"#.to_owned(),
                // r#"||foxbusiness.com/lifestyle/*"#.to_owned(),
                // r#"||foxbusiness.com/markets/*"#.to_owned(),
                // r#"||foxbusiness.com/money/*"#.to_owned(),
                // r#"||foxbusiness.com/politics/*"#.to_owned(),
                // r#"||foxbusiness.com/sports/*"#.to_owned(),
                // r#"||foxbusiness.com/technology/*"#.to_owned(),
                r#"||foxnews.com/auto/*"#.to_owned(),
                r#"||foxnews.com/entertainment/*"#.to_owned(),
                r#"||foxnews.com/faith-values/*"#.to_owned(),
                r#"||foxnews.com/food-drink/*"#.to_owned(),
                r#"||foxnews.com/great-outdoors/*"#.to_owned(),
                r#"||foxnews.com/health/*"#.to_owned(),
                r#"||foxnews.com/lifestyle/*"#.to_owned(),
                r#"||foxnews.com/media/*"#.to_owned(),
                r#"||foxnews.com/opinion/*"#.to_owned(),
                r#"||foxnews.com/politics/*"#.to_owned(),
                r#"||foxnews.com/real-estate/*"#.to_owned(),
                r#"||foxnews.com/science/*"#.to_owned(),
                r#"||foxnews.com/sports/*"#.to_owned(),
                r#"||foxnews.com/tech/*"#.to_owned(),
                r#"||foxnews.com/travel/*"#.to_owned(),
                r#"||foxnews.com/us/*"#.to_owned(),
                r#"||foxnews.com/world/*"#.to_owned(),
            ],
            declarative_rewrite: Some(RewriteRules {
                main_content: vec!["article".to_owned()],
                main_content_cleanup: vec![
                    ".sidebar".to_owned(),
                    ".article-social".to_owned(),
                    ".author-headshot".to_owned(),
                ],
                ..RewriteRules::default()
            }),
        });

        self.add_configuration(SpeedReaderConfig {
            domain: "forbes.com".to_owned(),
            url_rules: vec![
                r#"/forbes.com\/sites\/\w+\/(\d){4}\/(\d){2}\/(\d){2}\/\w+/"#.to_owned(),
            ],
            declarative_rewrite: Some(RewriteRules {
                main_content: vec!["article > main".to_owned(), ".body-container".to_owned()],
                main_content_cleanup: vec![
                    ".article-footer".to_owned(),
                    ".disqus-module".to_owned(),
                    ".article-sharing".to_owned(),
                    "sharing".to_owned(),
                    ".fs-author-avatar".to_owned(),
                    ".fs-icon".to_owned(),
                    ".contrib-bio button".to_owned(),
                    ".contrib-bio .contributor-about__initial-description".to_owned(),
                    "fbs-ad".to_owned(),
                    "#speechkit-io-iframe".to_owned(),
                ],
                ..RewriteRules::default()
            }),
        });

        self.add_configuration(SpeedReaderConfig {
            domain: "cnbc.com".to_owned(),
            url_rules: vec![
                r#"/cnbc.com\/(\d){4}\/(\d){2}\/(\d){2}\/.*.html/"#.to_owned(),
                r#"||cnbc.com/select/*/"#.to_owned(),
            ],
            declarative_rewrite: Some(RewriteRules {
            main_content: vec![
                "#main-article-header".to_owned(),
                "[data-module=\"ArticleBody\"]".to_owned(),
            ],
            main_content_cleanup: vec![
                ".InlineVideo-videoEmbed".to_owned()
            ],
            delazify: false,
            fix_embeds: false,
            content_script: Some(r#"<script>
              [...document.querySelectorAll("figure")].map(f => {
                let imgid = f.id.replace("ArticleBody-InlineImage-", "");
                f.querySelector("img").src = "https://image.cnbcfm.com/api/v1/image/"+imgid+"-.jpeg?w=678";
              })
            </script>"#.to_owned()),
            preprocess: vec![
                AttributeRewrite {
                    selector: "[id^=\"ArticleBody-InlineImage\"]".to_owned(),
                    attribute: None,
                    element_name: "figure".to_owned()
                },
                AttributeRewrite {
                    selector: "[id^=\"ArticleBody-InlineImage\"] .lazyload-placeholder".to_owned(),
                    attribute: None,
                    element_name: "img".to_owned()
                },
                AttributeRewrite {
                    selector: "[id^=\"ArticleBody-InlineImage\"] > div > div:not([class*=\"imagePlaceholder\"])".to_owned(),
                    attribute: None,
                    element_name: "figcaption".to_owned()
                }
            ],
        })});

        self.add_configuration(SpeedReaderConfig {
            domain: "usatoday.com".to_owned(),
            url_rules: vec![r#"||usatoday.com/story/*"#.to_owned()],
            declarative_rewrite: Some(RewriteRules {
                main_content: vec!["article".to_owned(), ".article-wrapper".to_owned()],
                main_content_cleanup: vec![
                    ".gnt_ss".to_owned(),
                    "svg".to_owned(),
                    "custom-style".to_owned(),
                ],
                preprocess: vec![
                    AttributeRewrite {
                        selector: "button[data-c-vpattrs]".to_owned(),
                        attribute: Some(("id".to_owned(), "id".to_owned())),
                        element_name: "div".to_owned(),
                    },
                    AttributeRewrite {
                        selector: "slide".to_owned(),
                        attribute: Some(("original".to_owned(), "src".to_owned())),
                        element_name: "img".to_owned(),
                    },
                ],
                ..RewriteRules::default()
            }),
        });

        self.add_configuration(SpeedReaderConfig {
            domain: "wsj.com".to_owned(),
            url_rules: vec![
                // r#"||www.barrons.com/articles/"#.to_owned(),
                r#"||www.wsj.com/articles/"#.to_owned(),
                // r#"||marketwatch.com/story/*"#.to_owned(),
            ],
            declarative_rewrite: Some(RewriteRules {
                main_content: vec!["article > main".to_owned()],
                main_content_cleanup: vec![
                    "#saving-united-coupon-list".to_owned(),
                    ".author-info".to_owned(),
                ],
                ..RewriteRules::default()
            }),
        });

        self.add_configuration(SpeedReaderConfig {
            domain: "reuters.com".to_owned(),
            url_rules: vec![r#"||reuters.com/article/*"#.to_owned()],
            declarative_rewrite: Some(RewriteRules {
                main_content: vec![
                    ".ArticleHeader_container".to_owned(),
                    ".StandardArticleBody_body".to_owned(),
                ],
                main_content_cleanup: vec![
                    ".SmallImage_small-image".to_owned(),
                    "[class$=expand-button]".to_owned(),
                    ".Slideshow_caption".to_owned(),
                    "[role=button]".to_owned(),
                ],
                content_script: Some(
                    r#"<script>
                [...document.querySelectorAll(".LazyImage_container img")]
                    .map(i => i.src = i.src.replace(/\&w=\d+/, "&w=600"));
            </script>"#
                        .to_owned(),
                ),
                ..RewriteRules::default()
            }),
        });

        self.add_configuration(SpeedReaderConfig {
            domain: "nypost.com".to_owned(),
            url_rules: vec![r#"/nypost.com\/(\d){4}\/(\d){2}\/(\d){2}\/.*/"#.to_owned()],
            declarative_rewrite: Some(RewriteRules {
                main_content: vec![".article-header".to_owned(), ".slide".to_owned()],
                main_content_cleanup: vec![
                    ".no-mobile".to_owned(),
                    ".author-contact".to_owned(),
                    ".sharedaddy".to_owned(),
                    ".author-flyout".to_owned(),
                ],
                ..RewriteRules::default()
            }),
        });

        self.add_configuration(SpeedReaderConfig {
            domain: "chron.com".to_owned(),
            url_rules: vec!["||chron.com/*/article/".to_owned()],
            declarative_rewrite: Some(RewriteRules {
                main_content: vec![".article-title".to_owned(), ".article-body".to_owned()],
                main_content_cleanup: vec![
                    ".hidden".to_owned(),
                    ".control-panel".to_owned(),
                    ".article-body > script".to_owned(),
                    ".caption-truncated".to_owned(),
                ],
                preprocess: vec![AttributeRewrite {
                    selector: "li.hst-resgalleryitem".to_owned(),
                    attribute: None,
                    element_name: "div".to_owned(),
                }],
                ..RewriteRules::default()
            }),
        });

        self.add_configuration(SpeedReaderConfig {
            domain: "nbcnews.com".to_owned(),
            url_rules: vec![
                "||nbcnews.com/*-n*".to_owned(),
                "@@||nbcnews.com/video".to_owned(),
            ],
            declarative_rewrite: Some(RewriteRules {
                main_content: vec![
                    ".article header".to_owned(),
                    ".article article".to_owned(),
                    ".article figure".to_owned(),
                ],
                main_content_cleanup: vec![
                    ".article article svg".to_owned(),
                    "[data-test=newsletter-signup]".to_owned(),
                    "#emailSignup".to_owned(),
                    ".ad-container".to_owned(),
                ],
                ..RewriteRules::default()
            }),
        });

        self.add_configuration(SpeedReaderConfig {
            domain: "dw.com".to_owned(),
            url_rules: vec!["||dw.com/*/a-*".to_owned()],
            declarative_rewrite: Some(RewriteRules {
                main_content: vec!["#bodyContent".to_owned()],
                main_content_cleanup: vec![
                    "[class$=Teaser]".to_owned(),
                    ".video".to_owned(),
                    ".relatedContent".to_owned(),
                    ".smallList".to_owned(),
                    "#sharing-bar".to_owned(),
                ],
                ..RewriteRules::default()
            }),
        });

        self.add_configuration(SpeedReaderConfig {
            domain: "time.com".to_owned(),
            url_rules: vec![r#"/time.com\/(\d){6,}\/.*/"#.to_owned()],
            declarative_rewrite: Some(RewriteRules {
                main_content: vec!["main.article".to_owned()],
                main_content_cleanup: vec![
                    ".edit-link".to_owned(),
                    ".most-popular-feed".to_owned(),
                    ".inline-recirc".to_owned(),
                    ".newsletter-callout".to_owned(),
                    ".article-bottom".to_owned(),
                    ".article-small-sidebar".to_owned(),
                    ".ad".to_owned(),
                    ".component.video video:not([poster])".to_owned(),
                ],
                preprocess: vec![AttributeRewrite {
                    selector: "noscript".to_owned(),
                    attribute: None,
                    element_name: "div".to_owned(),
                }],
                ..RewriteRules::default()
            }),
        });

        self.add_configuration(SpeedReaderConfig {
            domain: "cbsnews.com".to_owned(),
            url_rules: vec![
                "||cbsnews.com/news/*".to_owned(),
                "@@||cbsnews.com/live".to_owned(),
                "@@||cbsnews.com/video".to_owned(),
            ],
            declarative_rewrite: Some(RewriteRules {
                main_content: vec!["article.content".to_owned(), "article.article".to_owned()],
                main_content_cleanup: vec![
                    ".sharebar".to_owned(),
                    ".content__cta".to_owned(),
                    "figure .embed__content--draggable".to_owned(),
                    "figure svg".to_owned(),
                    "script".to_owned(),
                    "[data-component=socialLinks]".to_owned(),
                    "[data-component=sharebar]".to_owned(),
                ],
                preprocess: vec![AttributeRewrite {
                    selector: "link[as=\"image\"]".to_owned(),
                    attribute: Some(("href".to_owned(), "src".to_owned())),
                    element_name: "img".to_owned(),
                }],
                ..RewriteRules::default()
            }),
        });

        self.add_configuration(SpeedReaderConfig {
            domain: "thedailybeast.com".to_owned(),
            url_rules: vec![
              "@@||thedailybeast.com/category/".to_owned(),
              r#"/thedailybeast\.com\/(\w+-)+/"#.to_owned(),
            ],
            declarative_rewrite: Some(RewriteRules {
            main_content: vec!["article.Story".to_owned(), "body > div > script:not([src]):not([type])".to_owned()],
            main_content_cleanup: vec![
                ".StandardHeader__share-buttons".to_owned(),
                ".StoryFooter".to_owned(),
                ".PullQuote__logo-icon".to_owned(),
                ".PullQuote__top-line".to_owned(),
                ".PullQuote__big-quote".to_owned(),
                "figure svg".to_owned(),
                ".SimpleAd".to_owned(),
                ".Byline__photo-link".to_owned(),
            ],
            delazify: true,
            fix_embeds: false,
            content_script: Some(r#"<script>
            [...document.querySelectorAll(".Body .LazyLoad")].map((div, i) => {
                let lazyLoad = window.__INITIAL_STATE__.body.cards.filter(c => c[0] === "pt-image" || c[0] === "pt-video-card")[i];
                if (!lazyLoad || lazyLoad[0] !== "pt-image") return;
                let figure = document.createElement("figure");
                figure.innerHTML = '<img src="https://img.thedailybeast.com/image/upload/c_crop/dpr_1.0/c_limit,w_600/fl_lossy,q_auto/' 
                    + lazyLoad[1].public_id + '"><figcaption>' 
                    + lazyLoad[1].title + ' Credit: ' 
                    + lazyLoad[1].credit + '</figcaption>';
                div.appendChild(figure);
            })
            </script>"#.to_owned()),
            preprocess: vec![
                AttributeRewrite {
                    selector: ".PullQuote".to_owned(),
                    attribute: None,
                    element_name: "blockquote".to_owned()
                }
            ],
        })});

        self.add_configuration(SpeedReaderConfig {
            domain: "businessinsider.com".to_owned(),
            url_rules: vec![r#"/businessinsider\.com\/(\w+-)+(\d){4}-(\d)/"#.to_owned()],
            declarative_rewrite: Some(RewriteRules {
                main_content: vec![
                    ".post-headline:nth".to_owned(),
                    ".byline-wrapper".to_owned(),
                    "#l-content".to_owned(),
                    ".container figure".to_owned(),
                ],
                main_content_cleanup: vec![
                    ".share-wrapper".to_owned(),
                    ".ad".to_owned(),
                    ".category-tagline".to_owned(),
                    ".popular-video".to_owned(),
                    "figure .lazy-image".to_owned(),
                    "figure .lazy-blur".to_owned(),
                ],
                preprocess: vec![AttributeRewrite {
                    selector: "figure noscript".to_owned(),
                    attribute: None,
                    element_name: "div".to_owned(),
                }],
                ..RewriteRules::default()
            }),
        });
    }
}

#[cfg(test)]
mod test {
    use super::*;

    #[test]
    pub fn default_whitelist_no_config() {
        let whitelist = Whitelist::default();
        assert!(whitelist.map.is_empty());
        let config = whitelist.get_configuration("example.com");
        assert!(config.is_none());
    }

    #[test]
    pub fn get_some_configuration() {
        let mut whitelist = Whitelist::default();
        whitelist.add_configuration(SpeedReaderConfig {
            domain: "example.com".to_owned(),
            url_rules: vec![
                r#"||example.com/article"#.to_owned(),
                r#"@@||example.com/article/video"#.to_owned(),
            ],
            declarative_rewrite: None,
        });
        let config = whitelist.get_configuration("example.com");
        assert!(config.is_some());
    }

    #[test]
    pub fn get_some_subdomain_configuration() {
        let mut whitelist = Whitelist::default();
        whitelist.add_configuration(SpeedReaderConfig {
            domain: "example.com".to_owned(),
            url_rules: vec![
                r#"||example.com/article"#.to_owned(),
                r#"@@||example.com/article/video"#.to_owned(),
            ],
            declarative_rewrite: None,
        });
        let config = whitelist.get_configuration("www.example.com");
        assert!(config.is_some());
    }

    #[test]
    pub fn url_rules_collected() {
        let mut whitelist = Whitelist::default();
        whitelist.add_configuration(SpeedReaderConfig {
            domain: "example.com".to_owned(),
            url_rules: vec![
                r#"||example.com/article"#.to_owned(),
                r#"@@||example.com/article/video"#.to_owned(),
            ],
            declarative_rewrite: None,
        });
        whitelist.add_configuration(SpeedReaderConfig {
            domain: "example.net".to_owned(),
            url_rules: vec![r#"||example.net/article"#.to_owned()],
            declarative_rewrite: None,
        });
        let rules = whitelist.get_url_rules();
        assert_eq!(rules.len(), 3);
    }

    #[test]
    pub fn conflicting_insert_overrides() {
        let mut whitelist = Whitelist::default();
        whitelist.add_configuration(SpeedReaderConfig {
            domain: "example.com".to_owned(),
            url_rules: vec![
                r#"||example.com/article"#.to_owned(),
                r#"@@||example.com/article/video"#.to_owned(),
            ],
            declarative_rewrite: None,
        });
        whitelist.add_configuration(SpeedReaderConfig {
            domain: "example.com".to_owned(),
            url_rules: vec![r#"||example.com/news"#.to_owned()],
            declarative_rewrite: None,
        });
        assert_eq!(whitelist.map.len(), 1);
        let config = whitelist.get_configuration("example.com");
        assert!(config.is_some());
        assert_eq!(
            config.unwrap().url_rules,
            vec!["||example.com/news".to_owned()]
        );
    }
}
