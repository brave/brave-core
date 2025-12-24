//! Support for parsing advisories from a Markdown file
//! (a.k.a. "V3 advisory format")

use crate::error::{Error, ErrorKind};

/// Parts of a parsed advisory
#[derive(Copy, Clone, Debug)]
pub struct Parts<'a> {
    /// TOML front matter
    pub front_matter: &'a str,

    /// Markdown without front matter
    pub markdown: &'a str,

    /// Advisory title
    pub title: &'a str,

    /// Advisory description (markdown)
    pub description: &'a str,
}

impl<'a> Parts<'a> {
    /// Parse a Markdown advisory into its component parts
    pub fn parse(advisory_data: &'a str) -> Result<Self, Error> {
        if !advisory_data.starts_with("```toml") {
            let context = if advisory_data.len() > 20 {
                &advisory_data[..20]
            } else {
                advisory_data
            };

            fail!(
                ErrorKind::Parse,
                "unexpected start of advisory: \"{}\"",
                context
            )
        }

        let toml_end = advisory_data.find("\n```").ok_or_else(|| {
            Error::new(
                ErrorKind::Parse,
                "couldn't find end of TOML front matter in advisory",
            )
        })?;

        let front_matter = advisory_data[7..toml_end].trim_start().trim_end();
        let markdown = advisory_data[(toml_end + 4)..].trim_start();

        if !markdown.starts_with("# ") {
            fail!(
                ErrorKind::Parse,
                "Expected # header after TOML front matter"
            );
        }

        let next_newline = markdown.find('\n').ok_or_else(|| {
            Error::new(
                ErrorKind::Parse,
                "no Markdown body (i.e. description) found",
            )
        })?;

        let title = markdown[2..next_newline].trim_end();
        let description = markdown[(next_newline + 1)..].trim_start().trim_end();

        Ok(Self {
            front_matter,
            markdown,
            title,
            description,
        })
    }
}
