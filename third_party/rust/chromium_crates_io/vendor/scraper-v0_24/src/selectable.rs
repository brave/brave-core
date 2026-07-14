//! Provides the [`Selectable`] to abstract over collections of elements

use crate::{
    element_ref::{self, ElementRef},
    html::{self, Html},
    selector::Selector,
};

/// Trait to abstract over collections of elements to which a [CSS selector][Selector] can be applied
///
/// The mainly enables writing helper functions which are generic over [`Html`] and [`ElementRef`], e.g.
///
/// ```
/// use scraper::{selectable::Selectable, selector::Selector};
///
/// fn text_of_first_match<'a, S>(selectable: S, selector: &Selector) -> Option<String>
/// where
///     S: Selectable<'a>,
/// {
///     selectable.select(selector).next().map(|element| element.text().collect())
/// }
/// ```
pub trait Selectable<'a> {
    /// Iterator over [element references][ElementRef] matching a [CSS selector[Selector]
    type Select<'b>: Iterator<Item = ElementRef<'a>>;

    /// Applies the given `selector` to the collection of elements represented by `self`
    fn select(self, selector: &Selector) -> Self::Select<'_>;
}

impl<'a> Selectable<'a> for &'a Html {
    type Select<'b> = html::Select<'a, 'b>;

    fn select(self, selector: &Selector) -> Self::Select<'_> {
        Html::select(self, selector)
    }
}

impl<'a> Selectable<'a> for ElementRef<'a> {
    type Select<'b> = element_ref::Select<'a, 'b>;

    fn select(self, selector: &Selector) -> Self::Select<'_> {
        ElementRef::select(&self, selector)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn select_one<'a, S>(selectable: S, selector: &Selector) -> Option<ElementRef<'a>>
    where
        S: Selectable<'a>,
    {
        selectable.select(selector).next()
    }

    #[test]
    fn html_and_element_ref_are_selectable() {
        let fragment = Html::parse_fragment(
            r#"<select class="foo"><option value="bar">foobar</option></select>"#,
        );

        let selector = Selector::parse("select.foo").unwrap();
        let element = select_one(&fragment, &selector).unwrap();

        let selector = Selector::parse("select.foo option[value='bar']").unwrap();
        let _element = select_one(element, &selector).unwrap();
    }
}
