use rstest::rstest;
use time::format_description::{BorrowedFormatItem, Component, OwnedFormatItem, modifier};

#[rstest]
fn borrowed_format_item_component_conversions() {
    let component = Component::CalendarYearFullExtendedRange(
        modifier::CalendarYearFullExtendedRange::default(),
    );
    let item = BorrowedFormatItem::from(component);
    assert!(matches!(item, BorrowedFormatItem::Component(inner) if inner == component));
    assert_eq!(Component::try_from(item), Ok(component));
    assert!(Component::try_from(BorrowedFormatItem::StringLiteral("")).is_err());
    assert!(<&[BorrowedFormatItem<'_>]>::try_from(BorrowedFormatItem::StringLiteral("")).is_err());
}

#[rstest]
fn borrowed_format_item_compound_conversions() {
    let compound = [BorrowedFormatItem::StringLiteral("")].as_slice();
    let item = BorrowedFormatItem::from(compound);
    assert!(matches!(item, BorrowedFormatItem::Compound(inner) if inner == compound));
    assert_eq!(<&[BorrowedFormatItem<'_>]>::try_from(item), Ok(compound));
}

#[rstest]
fn borrowed_format_item_equality() {
    let component = Component::CalendarYearFullExtendedRange(
        modifier::CalendarYearFullExtendedRange::default(),
    );
    let compound = [BorrowedFormatItem::StringLiteral("")].as_slice();
    let component_item = BorrowedFormatItem::from(component);
    let compound_item = BorrowedFormatItem::from(compound);

    assert_eq!(component, component_item);
    assert_eq!(component_item, component);
    assert_eq!(compound, compound_item);
    assert_eq!(compound_item, compound);
}

#[rstest]
fn owned_format_item_component_conversions() {
    let component = Component::CalendarYearFullExtendedRange(
        modifier::CalendarYearFullExtendedRange::default(),
    );
    let item = OwnedFormatItem::from(component);
    assert!(matches!(item, OwnedFormatItem::Component(inner) if inner == component));
    assert_eq!(Component::try_from(item), Ok(component));
    assert!(
        Component::try_from(OwnedFormatItem::StringLiteral(
            "".to_owned().into_boxed_str()
        ))
        .is_err()
    );
    assert!(
        Vec::<OwnedFormatItem>::try_from(OwnedFormatItem::StringLiteral(
            "".to_owned().into_boxed_str()
        ))
        .is_err()
    );
}

#[rstest]
fn owned_format_item_compound_conversions() {
    let compound = vec![OwnedFormatItem::StringLiteral(
        "".to_owned().into_boxed_str(),
    )];
    let item = OwnedFormatItem::from(compound.clone());
    assert!(matches!(item.clone(), OwnedFormatItem::Compound(inner) if inner.to_vec() == compound));
    assert_eq!(Vec::<OwnedFormatItem>::try_from(item), Ok(compound));
}

#[rstest]
fn owned_format_item_equality() {
    let component = Component::CalendarYearFullExtendedRange(
        modifier::CalendarYearFullExtendedRange::default(),
    );
    let compound = OwnedFormatItem::from([BorrowedFormatItem::StringLiteral("")].as_slice());
    let component_item = OwnedFormatItem::from(component);

    assert_eq!(component, component_item);
    assert_eq!(component_item, component);
    assert_eq!(
        compound,
        [OwnedFormatItem::StringLiteral(
            "".to_owned().into_boxed_str()
        )]
        .as_slice()
    );
    assert_eq!(
        [OwnedFormatItem::StringLiteral(
            "".to_owned().into_boxed_str()
        )]
        .as_slice(),
        compound
    );
}
