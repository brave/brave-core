mod numerical_duration {
    use rstest::rstest;
    use time::SignedDuration;
    use time::ext::NumericalDuration;

    #[rstest]
    #[case(5.nanoseconds(), SignedDuration::nanoseconds(5))]
    #[case(5.microseconds(), SignedDuration::microseconds(5))]
    #[case(5.milliseconds(), SignedDuration::milliseconds(5))]
    #[case(5.seconds(), SignedDuration::seconds(5))]
    #[case(5.minutes(), SignedDuration::minutes(5))]
    #[case(5.hours(), SignedDuration::hours(5))]
    #[case(5.days(), SignedDuration::days(5))]
    #[case(5.weeks(), SignedDuration::weeks(5))]
    fn unsigned(#[case] ext_trait: SignedDuration, #[case] expected: SignedDuration) {
        assert_eq!(ext_trait, expected);
    }

    #[rstest]
    #[case((-5).nanoseconds(), SignedDuration::nanoseconds(-5))]
    #[case((-5).microseconds(), SignedDuration::microseconds(-5))]
    #[case((-5).milliseconds(), SignedDuration::milliseconds(-5))]
    #[case((-5).seconds(), SignedDuration::seconds(-5))]
    #[case((-5).minutes(), SignedDuration::minutes(-5))]
    #[case((-5).hours(), SignedDuration::hours(-5))]
    #[case((-5).days(), SignedDuration::days(-5))]
    #[case((-5).weeks(), SignedDuration::weeks(-5))]
    fn signed(#[case] ext_trait: SignedDuration, #[case] expected: SignedDuration) {
        assert_eq!(ext_trait, expected);
    }

    #[rstest]
    #[case::truncate_not_round(1.9.nanoseconds(), SignedDuration::nanoseconds(1))]
    #[case(1.0.nanoseconds(), SignedDuration::nanoseconds(1))]
    #[case(1.0.microseconds(), SignedDuration::microseconds(1))]
    #[case(1.0.milliseconds(), SignedDuration::milliseconds(1))]
    #[case(1.0.seconds(), SignedDuration::seconds(1))]
    #[case(1.0.minutes(), SignedDuration::minutes(1))]
    #[case(1.0.hours(), SignedDuration::hours(1))]
    #[case(1.0.days(), SignedDuration::days(1))]
    #[case(1.0.weeks(), SignedDuration::weeks(1))]
    #[case(1.5.nanoseconds(), SignedDuration::nanoseconds(1))]
    #[case(1.5.microseconds(), SignedDuration::nanoseconds(1_500))]
    #[case(1.5.milliseconds(), SignedDuration::microseconds(1_500))]
    #[case(1.5.seconds(), SignedDuration::milliseconds(1_500))]
    #[case(1.5.minutes(), SignedDuration::seconds(90))]
    #[case(1.5.hours(), SignedDuration::minutes(90))]
    #[case(1.5.days(), SignedDuration::hours(36))]
    #[case(1.5.weeks(), SignedDuration::hours(252))]
    fn float(#[case] ext_trait: SignedDuration, #[case] expected: SignedDuration) {
        assert_eq!(ext_trait, expected);
    }

    #[rstest]
    #[case(2.seconds() + 500.milliseconds(), 2_500.milliseconds())]
    #[case(2.seconds() - 500.milliseconds(), 1_500.milliseconds())]
    fn arithmetic(#[case] arithmetic_: SignedDuration, #[case] expected: SignedDuration) {
        assert_eq!(arithmetic_, expected);
    }
}

mod numerical_std_duration {
    use std::time::Duration as StdDuration;

    use rstest::rstest;
    use time::ext::NumericalStdDuration;

    #[rstest]
    #[case(5.std_nanoseconds(), StdDuration::from_nanos(5))]
    #[case(5.std_microseconds(), StdDuration::from_micros(5))]
    #[case(5.std_milliseconds(), StdDuration::from_millis(5))]
    #[case(5.std_seconds(), StdDuration::from_secs(5))]
    #[case(5.std_minutes(), StdDuration::from_secs(5 * 60))]
    #[case(5.std_hours(), StdDuration::from_secs(5 * 3_600))]
    #[case(5.std_days(), StdDuration::from_secs(5 * 86_400))]
    #[case(5.std_weeks(), StdDuration::from_secs(5 * 604_800))]
    fn unsigned(#[case] ext_trait: StdDuration, #[case] expected: StdDuration) {
        assert_eq!(ext_trait, expected);
    }

    #[rstest]
    #[case::truncate_not_round(1.9.std_nanoseconds(), StdDuration::from_nanos(1))]
    #[case(1.0.std_nanoseconds(), StdDuration::from_nanos(1))]
    #[case(1.0.std_microseconds(), StdDuration::from_micros(1))]
    #[case(1.0.std_milliseconds(), StdDuration::from_millis(1))]
    #[case(1.0.std_seconds(), StdDuration::from_secs(1))]
    #[case(1.0.std_minutes(), StdDuration::from_secs(60))]
    #[case(1.0.std_hours(), StdDuration::from_secs(3_600))]
    #[case(1.0.std_days(), StdDuration::from_secs(86_400))]
    #[case(1.0.std_weeks(), StdDuration::from_secs(604_800))]
    #[case(1.5.std_nanoseconds(), StdDuration::from_nanos(1))]
    #[case(1.5.std_microseconds(), StdDuration::from_nanos(1_500))]
    #[case(1.5.std_milliseconds(), StdDuration::from_micros(1_500))]
    #[case(1.5.std_seconds(), StdDuration::from_millis(1_500))]
    #[case(1.5.std_minutes(), StdDuration::from_secs(90))]
    #[case(1.5.std_hours(), StdDuration::from_secs(90 * 60))]
    #[case(1.5.std_days(), StdDuration::from_secs(36 * 3_600))]
    #[case(1.5.std_weeks(), StdDuration::from_secs(252 * 3_600))]
    fn float(#[case] ext_trait: StdDuration, #[case] expected: StdDuration) {
        assert_eq!(ext_trait, expected);
    }

    #[rstest]
    #[case(2.std_seconds() + 500.std_milliseconds(), 2_500.std_milliseconds())]
    #[case(2.std_seconds() - 500.std_milliseconds(), 1_500.std_milliseconds())]
    fn arithmetic(#[case] arithmetic_: StdDuration, #[case] expected: StdDuration) {
        assert_eq!(arithmetic_, expected);
    }
}
