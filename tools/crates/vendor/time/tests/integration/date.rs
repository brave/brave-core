use std::cmp::Ordering;
use std::time::Duration as StdDuration;

use rstest::rstest;
use time::Weekday::*;
use time::ext::{NumericalDuration, NumericalStdDuration};
use time::macros::{date, datetime, time};
use time::{Date, Month, PlainDateTime, SignedDuration, Weekday, util};

#[rstest]
#[case(date!(2020-02-03), "2020-02-03")]
fn debug(#[case] date: Date, #[case] expected: &str) {
    assert_eq!(format!("{date:?}"), expected);
}

#[rstest]
#[case(0, 52)]
#[case(1, 52)]
#[case(2, 52)]
#[case(3, 52)]
#[case(4, 53)]
#[case(5, 52)]
#[case(6, 52)]
#[case(7, 52)]
#[case(8, 52)]
#[case(9, 53)]
#[case(10, 52)]
#[case(11, 52)]
#[case(12, 52)]
#[case(13, 52)]
#[case(14, 52)]
#[case(15, 53)]
#[case(16, 52)]
#[case(17, 52)]
#[case(18, 52)]
#[case(19, 52)]
#[case(20, 53)]
#[case(21, 52)]
#[case(22, 52)]
#[case(23, 52)]
#[case(24, 52)]
#[case(25, 52)]
#[case(26, 53)]
#[case(27, 52)]
#[case(28, 52)]
#[case(29, 52)]
#[case(30, 52)]
#[case(31, 52)]
#[case(32, 53)]
#[case(33, 52)]
#[case(34, 52)]
#[case(35, 52)]
#[case(36, 52)]
#[case(37, 53)]
#[case(38, 52)]
#[case(39, 52)]
#[case(40, 52)]
#[case(41, 52)]
#[case(42, 52)]
#[case(43, 53)]
#[case(44, 52)]
#[case(45, 52)]
#[case(46, 52)]
#[case(47, 52)]
#[case(48, 53)]
#[case(49, 52)]
#[case(50, 52)]
#[case(51, 52)]
#[case(52, 52)]
#[case(53, 52)]
#[case(54, 53)]
#[case(55, 52)]
#[case(56, 52)]
#[case(57, 52)]
#[case(58, 52)]
#[case(59, 52)]
#[case(60, 53)]
#[case(61, 52)]
#[case(62, 52)]
#[case(63, 52)]
#[case(64, 52)]
#[case(65, 53)]
#[case(66, 52)]
#[case(67, 52)]
#[case(68, 52)]
#[case(69, 52)]
#[case(70, 52)]
#[case(71, 53)]
#[case(72, 52)]
#[case(73, 52)]
#[case(74, 52)]
#[case(75, 52)]
#[case(76, 53)]
#[case(77, 52)]
#[case(78, 52)]
#[case(79, 52)]
#[case(80, 52)]
#[case(81, 52)]
#[case(82, 53)]
#[case(83, 52)]
#[case(84, 52)]
#[case(85, 52)]
#[case(86, 52)]
#[case(87, 52)]
#[case(88, 53)]
#[case(89, 52)]
#[case(90, 52)]
#[case(91, 52)]
#[case(92, 52)]
#[case(93, 53)]
#[case(94, 52)]
#[case(95, 52)]
#[case(96, 52)]
#[case(97, 52)]
#[case(98, 52)]
#[case(99, 53)]
#[case(100, 52)]
#[case(101, 52)]
#[case(102, 52)]
#[case(103, 52)]
#[case(104, 52)]
#[case(105, 53)]
#[case(106, 52)]
#[case(107, 52)]
#[case(108, 52)]
#[case(109, 52)]
#[case(110, 52)]
#[case(111, 53)]
#[case(112, 52)]
#[case(113, 52)]
#[case(114, 52)]
#[case(115, 52)]
#[case(116, 53)]
#[case(117, 52)]
#[case(118, 52)]
#[case(119, 52)]
#[case(120, 52)]
#[case(121, 52)]
#[case(122, 53)]
#[case(123, 52)]
#[case(124, 52)]
#[case(125, 52)]
#[case(126, 52)]
#[case(127, 52)]
#[case(128, 53)]
#[case(129, 52)]
#[case(130, 52)]
#[case(131, 52)]
#[case(132, 52)]
#[case(133, 53)]
#[case(134, 52)]
#[case(135, 52)]
#[case(136, 52)]
#[case(137, 52)]
#[case(138, 52)]
#[case(139, 53)]
#[case(140, 52)]
#[case(141, 52)]
#[case(142, 52)]
#[case(143, 52)]
#[case(144, 53)]
#[case(145, 52)]
#[case(146, 52)]
#[case(147, 52)]
#[case(148, 52)]
#[case(149, 52)]
#[case(150, 53)]
#[case(151, 52)]
#[case(152, 52)]
#[case(153, 52)]
#[case(154, 52)]
#[case(155, 52)]
#[case(156, 53)]
#[case(157, 52)]
#[case(158, 52)]
#[case(159, 52)]
#[case(160, 52)]
#[case(161, 53)]
#[case(162, 52)]
#[case(163, 52)]
#[case(164, 52)]
#[case(165, 52)]
#[case(166, 52)]
#[case(167, 53)]
#[case(168, 52)]
#[case(169, 52)]
#[case(170, 52)]
#[case(171, 52)]
#[case(172, 53)]
#[case(173, 52)]
#[case(174, 52)]
#[case(175, 52)]
#[case(176, 52)]
#[case(177, 52)]
#[case(178, 53)]
#[case(179, 52)]
#[case(180, 52)]
#[case(181, 52)]
#[case(182, 52)]
#[case(183, 52)]
#[case(184, 53)]
#[case(185, 52)]
#[case(186, 52)]
#[case(187, 52)]
#[case(188, 52)]
#[case(189, 53)]
#[case(190, 52)]
#[case(191, 52)]
#[case(192, 52)]
#[case(193, 52)]
#[case(194, 52)]
#[case(195, 53)]
#[case(196, 52)]
#[case(197, 52)]
#[case(198, 52)]
#[case(199, 52)]
#[case(200, 52)]
#[case(201, 53)]
#[case(202, 52)]
#[case(203, 52)]
#[case(204, 52)]
#[case(205, 52)]
#[case(206, 52)]
#[case(207, 53)]
#[case(208, 52)]
#[case(209, 52)]
#[case(210, 52)]
#[case(211, 52)]
#[case(212, 53)]
#[case(213, 52)]
#[case(214, 52)]
#[case(215, 52)]
#[case(216, 52)]
#[case(217, 52)]
#[case(218, 53)]
#[case(219, 52)]
#[case(220, 52)]
#[case(221, 52)]
#[case(222, 52)]
#[case(223, 52)]
#[case(224, 53)]
#[case(225, 52)]
#[case(226, 52)]
#[case(227, 52)]
#[case(228, 52)]
#[case(229, 53)]
#[case(230, 52)]
#[case(231, 52)]
#[case(232, 52)]
#[case(233, 52)]
#[case(234, 52)]
#[case(235, 53)]
#[case(236, 52)]
#[case(237, 52)]
#[case(238, 52)]
#[case(239, 52)]
#[case(240, 53)]
#[case(241, 52)]
#[case(242, 52)]
#[case(243, 52)]
#[case(244, 52)]
#[case(245, 52)]
#[case(246, 53)]
#[case(247, 52)]
#[case(248, 52)]
#[case(249, 52)]
#[case(250, 52)]
#[case(251, 52)]
#[case(252, 53)]
#[case(253, 52)]
#[case(254, 52)]
#[case(255, 52)]
#[case(256, 52)]
#[case(257, 53)]
#[case(258, 52)]
#[case(259, 52)]
#[case(260, 52)]
#[case(261, 52)]
#[case(262, 52)]
#[case(263, 53)]
#[case(264, 52)]
#[case(265, 52)]
#[case(266, 52)]
#[case(267, 52)]
#[case(268, 53)]
#[case(269, 52)]
#[case(270, 52)]
#[case(271, 52)]
#[case(272, 52)]
#[case(273, 52)]
#[case(274, 53)]
#[case(275, 52)]
#[case(276, 52)]
#[case(277, 52)]
#[case(278, 52)]
#[case(279, 52)]
#[case(280, 53)]
#[case(281, 52)]
#[case(282, 52)]
#[case(283, 52)]
#[case(284, 52)]
#[case(285, 53)]
#[case(286, 52)]
#[case(287, 52)]
#[case(288, 52)]
#[case(289, 52)]
#[case(290, 52)]
#[case(291, 53)]
#[case(292, 52)]
#[case(293, 52)]
#[case(294, 52)]
#[case(295, 52)]
#[case(296, 53)]
#[case(297, 52)]
#[case(298, 52)]
#[case(299, 52)]
#[case(300, 52)]
#[case(301, 52)]
#[case(302, 52)]
#[case(303, 53)]
#[case(304, 52)]
#[case(305, 52)]
#[case(306, 52)]
#[case(307, 52)]
#[case(308, 53)]
#[case(309, 52)]
#[case(310, 52)]
#[case(311, 52)]
#[case(312, 52)]
#[case(313, 52)]
#[case(314, 53)]
#[case(315, 52)]
#[case(316, 52)]
#[case(317, 52)]
#[case(318, 52)]
#[case(319, 52)]
#[case(320, 53)]
#[case(321, 52)]
#[case(322, 52)]
#[case(323, 52)]
#[case(324, 52)]
#[case(325, 53)]
#[case(326, 52)]
#[case(327, 52)]
#[case(328, 52)]
#[case(329, 52)]
#[case(330, 52)]
#[case(331, 53)]
#[case(332, 52)]
#[case(333, 52)]
#[case(334, 52)]
#[case(335, 52)]
#[case(336, 53)]
#[case(337, 52)]
#[case(338, 52)]
#[case(339, 52)]
#[case(340, 52)]
#[case(341, 52)]
#[case(342, 53)]
#[case(343, 52)]
#[case(344, 52)]
#[case(345, 52)]
#[case(346, 52)]
#[case(347, 52)]
#[case(348, 53)]
#[case(349, 52)]
#[case(350, 52)]
#[case(351, 52)]
#[case(352, 52)]
#[case(353, 53)]
#[case(354, 52)]
#[case(355, 52)]
#[case(356, 52)]
#[case(357, 52)]
#[case(358, 52)]
#[case(359, 53)]
#[case(360, 52)]
#[case(361, 52)]
#[case(362, 52)]
#[case(363, 52)]
#[case(364, 53)]
#[case(365, 52)]
#[case(366, 52)]
#[case(367, 52)]
#[case(368, 52)]
#[case(369, 52)]
#[case(370, 53)]
#[case(371, 52)]
#[case(372, 52)]
#[case(373, 52)]
#[case(374, 52)]
#[case(375, 52)]
#[case(376, 53)]
#[case(377, 52)]
#[case(378, 52)]
#[case(379, 52)]
#[case(380, 52)]
#[case(381, 53)]
#[case(382, 52)]
#[case(383, 52)]
#[case(384, 52)]
#[case(385, 52)]
#[case(386, 52)]
#[case(387, 53)]
#[case(388, 52)]
#[case(389, 52)]
#[case(390, 52)]
#[case(391, 52)]
#[case(392, 53)]
#[case(393, 52)]
#[case(394, 52)]
#[case(395, 52)]
#[case(396, 52)]
#[case(397, 52)]
#[case(398, 53)]
#[case(399, 52)]
fn weeks_in_year_exhaustive(#[case] year: i32, #[case] expected: u8) {
    assert_eq!(util::weeks_in_year(year), expected);
}

// Test all dominical letters. For leap years, check the dates immediately preceding and after the
// leap day.

#[rstest]
#[case::dominical_a(date!(2023-01-01), 0)]
#[case::dominical_a(date!(2023-01-02), 1)]
#[case::dominical_a(date!(2023-01-03), 1)]
#[case::dominical_a(date!(2023-01-04), 1)]
#[case::dominical_a(date!(2023-01-05), 1)]
#[case::dominical_a(date!(2023-01-06), 1)]
#[case::dominical_a(date!(2023-01-07), 1)]
#[case::dominical_b(date!(2022-01-01), 0)]
#[case::dominical_b(date!(2022-01-02), 0)]
#[case::dominical_b(date!(2022-01-03), 1)]
#[case::dominical_b(date!(2022-01-04), 1)]
#[case::dominical_b(date!(2022-01-05), 1)]
#[case::dominical_b(date!(2022-01-06), 1)]
#[case::dominical_b(date!(2022-01-07), 1)]
#[case::dominical_c(date!(2021-01-01), 0)]
#[case::dominical_c(date!(2021-01-02), 0)]
#[case::dominical_c(date!(2021-01-03), 0)]
#[case::dominical_c(date!(2021-01-04), 1)]
#[case::dominical_c(date!(2021-01-05), 1)]
#[case::dominical_c(date!(2021-01-06), 1)]
#[case::dominical_c(date!(2021-01-07), 1)]
#[case::dominical_d(date!(2026-01-01), 0)]
#[case::dominical_d(date!(2026-01-02), 0)]
#[case::dominical_d(date!(2026-01-03), 0)]
#[case::dominical_d(date!(2026-01-04), 0)]
#[case::dominical_d(date!(2026-01-05), 1)]
#[case::dominical_d(date!(2026-01-06), 1)]
#[case::dominical_d(date!(2026-01-07), 1)]
#[case::dominical_e(date!(2025-01-01), 0)]
#[case::dominical_e(date!(2025-01-02), 0)]
#[case::dominical_e(date!(2025-01-03), 0)]
#[case::dominical_e(date!(2025-01-04), 0)]
#[case::dominical_e(date!(2025-01-05), 0)]
#[case::dominical_e(date!(2025-01-06), 1)]
#[case::dominical_e(date!(2025-01-07), 1)]
#[case::dominical_f(date!(2019-01-01), 0)]
#[case::dominical_f(date!(2019-01-02), 0)]
#[case::dominical_f(date!(2019-01-03), 0)]
#[case::dominical_f(date!(2019-01-04), 0)]
#[case::dominical_f(date!(2019-01-05), 0)]
#[case::dominical_f(date!(2019-01-06), 0)]
#[case::dominical_f(date!(2019-01-07), 1)]
#[case::dominical_g(date!(2018-01-01), 1)]
#[case::dominical_g(date!(2018-01-02), 1)]
#[case::dominical_g(date!(2018-01-03), 1)]
#[case::dominical_g(date!(2018-01-04), 1)]
#[case::dominical_g(date!(2018-01-05), 1)]
#[case::dominical_g(date!(2018-01-06), 1)]
#[case::dominical_g(date!(2018-01-07), 1)]
#[case::dominical_ag(date!(2012-01-01), 0)]
#[case::dominical_ag(date!(2012-01-02), 1)]
#[case::dominical_ag(date!(2012-01-03), 1)]
#[case::dominical_ag(date!(2012-01-04), 1)]
#[case::dominical_ag(date!(2012-01-05), 1)]
#[case::dominical_ag(date!(2012-01-06), 1)]
#[case::dominical_ag(date!(2012-01-07), 1)]
#[case::dominical_ag(date!(2012-02-28), 9)]
#[case::dominical_ag(date!(2012-02-29), 9)]
#[case::dominical_ag(date!(2012-03-01), 9)]
#[case::dominical_ag(date!(2012-03-02), 9)]
#[case::dominical_ag(date!(2012-03-03), 9)]
#[case::dominical_ag(date!(2012-03-04), 9)]
#[case::dominical_ag(date!(2012-03-05), 10)]
#[case::dominical_ag(date!(2012-03-06), 10)]
#[case::dominical_ag(date!(2012-03-07), 10)]
#[case::dominical_ba(date!(2028-01-01), 0)]
#[case::dominical_ba(date!(2028-01-02), 0)]
#[case::dominical_ba(date!(2028-01-03), 1)]
#[case::dominical_ba(date!(2028-01-04), 1)]
#[case::dominical_ba(date!(2028-01-05), 1)]
#[case::dominical_ba(date!(2028-01-06), 1)]
#[case::dominical_ba(date!(2028-01-07), 1)]
#[case::dominical_ba(date!(2028-02-28), 9)]
#[case::dominical_ba(date!(2028-02-29), 9)]
#[case::dominical_ba(date!(2028-03-01), 9)]
#[case::dominical_ba(date!(2028-03-02), 9)]
#[case::dominical_ba(date!(2028-03-03), 9)]
#[case::dominical_ba(date!(2028-03-04), 9)]
#[case::dominical_ba(date!(2028-03-05), 9)]
#[case::dominical_ba(date!(2028-03-06), 10)]
#[case::dominical_ba(date!(2028-03-07), 10)]
#[case::dominical_cb(date!(2016-01-01), 0)]
#[case::dominical_cb(date!(2016-01-02), 0)]
#[case::dominical_cb(date!(2016-01-03), 0)]
#[case::dominical_cb(date!(2016-01-04), 1)]
#[case::dominical_cb(date!(2016-01-05), 1)]
#[case::dominical_cb(date!(2016-01-06), 1)]
#[case::dominical_cb(date!(2016-01-07), 1)]
#[case::dominical_cb(date!(2016-02-28), 8)]
#[case::dominical_cb(date!(2016-02-29), 9)]
#[case::dominical_cb(date!(2016-03-01), 9)]
#[case::dominical_cb(date!(2016-03-02), 9)]
#[case::dominical_cb(date!(2016-03-03), 9)]
#[case::dominical_cb(date!(2016-03-04), 9)]
#[case::dominical_cb(date!(2016-03-05), 9)]
#[case::dominical_cb(date!(2016-03-06), 9)]
#[case::dominical_cb(date!(2016-03-07), 10)]
#[case::dominical_dc(date!(2032-01-01), 0)]
#[case::dominical_dc(date!(2032-01-02), 0)]
#[case::dominical_dc(date!(2032-01-03), 0)]
#[case::dominical_dc(date!(2032-01-04), 0)]
#[case::dominical_dc(date!(2032-01-05), 1)]
#[case::dominical_dc(date!(2032-01-06), 1)]
#[case::dominical_dc(date!(2032-01-07), 1)]
#[case::dominical_dc(date!(2032-02-28), 8)]
#[case::dominical_dc(date!(2032-02-29), 8)]
#[case::dominical_dc(date!(2032-03-01), 9)]
#[case::dominical_dc(date!(2032-03-02), 9)]
#[case::dominical_dc(date!(2032-03-03), 9)]
#[case::dominical_dc(date!(2032-03-04), 9)]
#[case::dominical_dc(date!(2032-03-05), 9)]
#[case::dominical_dc(date!(2032-03-06), 9)]
#[case::dominical_dc(date!(2032-03-07), 9)]
#[case::dominical_ed(date!(2020-01-01), 0)]
#[case::dominical_ed(date!(2020-01-02), 0)]
#[case::dominical_ed(date!(2020-01-03), 0)]
#[case::dominical_ed(date!(2020-01-04), 0)]
#[case::dominical_ed(date!(2020-01-05), 0)]
#[case::dominical_ed(date!(2020-01-06), 1)]
#[case::dominical_ed(date!(2020-01-07), 1)]
#[case::dominical_ed(date!(2020-02-28), 8)]
#[case::dominical_ed(date!(2020-02-29), 8)]
#[case::dominical_ed(date!(2020-03-01), 8)]
#[case::dominical_ed(date!(2020-03-02), 9)]
#[case::dominical_ed(date!(2020-03-03), 9)]
#[case::dominical_ed(date!(2020-03-04), 9)]
#[case::dominical_ed(date!(2020-03-05), 9)]
#[case::dominical_ed(date!(2020-03-06), 9)]
#[case::dominical_ed(date!(2020-03-07), 9)]
#[case::dominical_fe(date!(2036-01-01), 0)]
#[case::dominical_fe(date!(2036-01-02), 0)]
#[case::dominical_fe(date!(2036-01-03), 0)]
#[case::dominical_fe(date!(2036-01-04), 0)]
#[case::dominical_fe(date!(2036-01-05), 0)]
#[case::dominical_fe(date!(2036-01-06), 0)]
#[case::dominical_fe(date!(2036-01-07), 1)]
#[case::dominical_fe(date!(2036-02-28), 8)]
#[case::dominical_fe(date!(2036-02-29), 8)]
#[case::dominical_fe(date!(2036-03-01), 8)]
#[case::dominical_fe(date!(2036-03-02), 8)]
#[case::dominical_fe(date!(2036-03-03), 9)]
#[case::dominical_fe(date!(2036-03-04), 9)]
#[case::dominical_fe(date!(2036-03-05), 9)]
#[case::dominical_fe(date!(2036-03-06), 9)]
#[case::dominical_fe(date!(2036-03-07), 9)]
#[case::dominical_gf(date!(2024-01-01), 1)]
#[case::dominical_gf(date!(2024-01-02), 1)]
#[case::dominical_gf(date!(2024-01-03), 1)]
#[case::dominical_gf(date!(2024-01-04), 1)]
#[case::dominical_gf(date!(2024-01-05), 1)]
#[case::dominical_gf(date!(2024-01-06), 1)]
#[case::dominical_gf(date!(2024-01-07), 1)]
#[case::dominical_gf(date!(2024-02-28), 9)]
#[case::dominical_gf(date!(2024-02-29), 9)]
#[case::dominical_gf(date!(2024-03-01), 9)]
#[case::dominical_gf(date!(2024-03-02), 9)]
#[case::dominical_gf(date!(2024-03-03), 9)]
#[case::dominical_gf(date!(2024-03-04), 10)]
#[case::dominical_gf(date!(2024-03-05), 10)]
#[case::dominical_gf(date!(2024-03-06), 10)]
#[case::dominical_gf(date!(2024-03-07), 10)]
fn monday_based_week(#[case] date: Date, #[case] expected: u8) {
    assert_eq!(date.monday_based_week(), expected);
}

#[rstest]
#[case::dominical_a(date!(2023-01-01), 1)]
#[case::dominical_a(date!(2023-01-02), 1)]
#[case::dominical_a(date!(2023-01-03), 1)]
#[case::dominical_a(date!(2023-01-04), 1)]
#[case::dominical_a(date!(2023-01-05), 1)]
#[case::dominical_a(date!(2023-01-06), 1)]
#[case::dominical_a(date!(2023-01-07), 1)]
#[case::dominical_b(date!(2022-01-01), 0)]
#[case::dominical_b(date!(2022-01-02), 1)]
#[case::dominical_b(date!(2022-01-03), 1)]
#[case::dominical_b(date!(2022-01-04), 1)]
#[case::dominical_b(date!(2022-01-05), 1)]
#[case::dominical_b(date!(2022-01-06), 1)]
#[case::dominical_b(date!(2022-01-07), 1)]
#[case::dominical_c(date!(2021-01-01), 0)]
#[case::dominical_c(date!(2021-01-02), 0)]
#[case::dominical_c(date!(2021-01-03), 1)]
#[case::dominical_c(date!(2021-01-04), 1)]
#[case::dominical_c(date!(2021-01-05), 1)]
#[case::dominical_c(date!(2021-01-06), 1)]
#[case::dominical_c(date!(2021-01-07), 1)]
#[case::dominical_d(date!(2026-01-01), 0)]
#[case::dominical_d(date!(2026-01-02), 0)]
#[case::dominical_d(date!(2026-01-03), 0)]
#[case::dominical_d(date!(2026-01-04), 1)]
#[case::dominical_d(date!(2026-01-05), 1)]
#[case::dominical_d(date!(2026-01-06), 1)]
#[case::dominical_d(date!(2026-01-07), 1)]
#[case::dominical_e(date!(2025-01-01), 0)]
#[case::dominical_e(date!(2025-01-02), 0)]
#[case::dominical_e(date!(2025-01-03), 0)]
#[case::dominical_e(date!(2025-01-04), 0)]
#[case::dominical_e(date!(2025-01-05), 1)]
#[case::dominical_e(date!(2025-01-06), 1)]
#[case::dominical_e(date!(2025-01-07), 1)]
#[case::dominical_f(date!(2019-01-01), 0)]
#[case::dominical_f(date!(2019-01-02), 0)]
#[case::dominical_f(date!(2019-01-03), 0)]
#[case::dominical_f(date!(2019-01-04), 0)]
#[case::dominical_f(date!(2019-01-05), 0)]
#[case::dominical_f(date!(2019-01-06), 1)]
#[case::dominical_f(date!(2019-01-07), 1)]
#[case::dominical_g(date!(2018-01-01), 0)]
#[case::dominical_g(date!(2018-01-02), 0)]
#[case::dominical_g(date!(2018-01-03), 0)]
#[case::dominical_g(date!(2018-01-04), 0)]
#[case::dominical_g(date!(2018-01-05), 0)]
#[case::dominical_g(date!(2018-01-06), 0)]
#[case::dominical_g(date!(2018-01-07), 1)]
#[case::dominical_ag(date!(2012-01-01), 1)]
#[case::dominical_ag(date!(2012-01-02), 1)]
#[case::dominical_ag(date!(2012-01-03), 1)]
#[case::dominical_ag(date!(2012-01-04), 1)]
#[case::dominical_ag(date!(2012-01-05), 1)]
#[case::dominical_ag(date!(2012-01-06), 1)]
#[case::dominical_ag(date!(2012-01-07), 1)]
#[case::dominical_ag(date!(2012-02-28), 9)]
#[case::dominical_ag(date!(2012-02-29), 9)]
#[case::dominical_ag(date!(2012-03-01), 9)]
#[case::dominical_ag(date!(2012-03-02), 9)]
#[case::dominical_ag(date!(2012-03-03), 9)]
#[case::dominical_ag(date!(2012-03-04), 10)]
#[case::dominical_ag(date!(2012-03-05), 10)]
#[case::dominical_ag(date!(2012-03-06), 10)]
#[case::dominical_ag(date!(2012-03-07), 10)]
#[case::dominical_ba(date!(2028-01-01), 0)]
#[case::dominical_ba(date!(2028-01-02), 1)]
#[case::dominical_ba(date!(2028-01-03), 1)]
#[case::dominical_ba(date!(2028-01-04), 1)]
#[case::dominical_ba(date!(2028-01-05), 1)]
#[case::dominical_ba(date!(2028-01-06), 1)]
#[case::dominical_ba(date!(2028-01-07), 1)]
#[case::dominical_ba(date!(2028-02-28), 9)]
#[case::dominical_ba(date!(2028-02-29), 9)]
#[case::dominical_ba(date!(2028-03-01), 9)]
#[case::dominical_ba(date!(2028-03-02), 9)]
#[case::dominical_ba(date!(2028-03-03), 9)]
#[case::dominical_ba(date!(2028-03-04), 9)]
#[case::dominical_ba(date!(2028-03-05), 10)]
#[case::dominical_ba(date!(2028-03-06), 10)]
#[case::dominical_ba(date!(2028-03-07), 10)]
#[case::dominical_cb(date!(2016-01-01), 0)]
#[case::dominical_cb(date!(2016-01-02), 0)]
#[case::dominical_cb(date!(2016-01-03), 1)]
#[case::dominical_cb(date!(2016-01-04), 1)]
#[case::dominical_cb(date!(2016-01-05), 1)]
#[case::dominical_cb(date!(2016-01-06), 1)]
#[case::dominical_cb(date!(2016-01-07), 1)]
#[case::dominical_cb(date!(2016-02-28), 9)]
#[case::dominical_cb(date!(2016-02-29), 9)]
#[case::dominical_cb(date!(2016-03-01), 9)]
#[case::dominical_cb(date!(2016-03-02), 9)]
#[case::dominical_cb(date!(2016-03-03), 9)]
#[case::dominical_cb(date!(2016-03-04), 9)]
#[case::dominical_cb(date!(2016-03-05), 9)]
#[case::dominical_cb(date!(2016-03-06), 10)]
#[case::dominical_cb(date!(2016-03-07), 10)]
#[case::dominical_dc(date!(2032-01-01), 0)]
#[case::dominical_dc(date!(2032-01-02), 0)]
#[case::dominical_dc(date!(2032-01-03), 0)]
#[case::dominical_dc(date!(2032-01-04), 1)]
#[case::dominical_dc(date!(2032-01-05), 1)]
#[case::dominical_dc(date!(2032-01-06), 1)]
#[case::dominical_dc(date!(2032-01-07), 1)]
#[case::dominical_dc(date!(2032-02-28), 8)]
#[case::dominical_dc(date!(2032-02-29), 9)]
#[case::dominical_dc(date!(2032-03-01), 9)]
#[case::dominical_dc(date!(2032-03-02), 9)]
#[case::dominical_dc(date!(2032-03-03), 9)]
#[case::dominical_dc(date!(2032-03-04), 9)]
#[case::dominical_dc(date!(2032-03-05), 9)]
#[case::dominical_dc(date!(2032-03-06), 9)]
#[case::dominical_dc(date!(2032-03-07), 10)]
#[case::dominical_ed(date!(2020-01-01), 0)]
#[case::dominical_ed(date!(2020-01-02), 0)]
#[case::dominical_ed(date!(2020-01-03), 0)]
#[case::dominical_ed(date!(2020-01-04), 0)]
#[case::dominical_ed(date!(2020-01-05), 1)]
#[case::dominical_ed(date!(2020-01-06), 1)]
#[case::dominical_ed(date!(2020-01-07), 1)]
#[case::dominical_ed(date!(2020-02-28), 8)]
#[case::dominical_ed(date!(2020-02-29), 8)]
#[case::dominical_ed(date!(2020-03-01), 9)]
#[case::dominical_ed(date!(2020-03-02), 9)]
#[case::dominical_ed(date!(2020-03-03), 9)]
#[case::dominical_ed(date!(2020-03-04), 9)]
#[case::dominical_ed(date!(2020-03-05), 9)]
#[case::dominical_ed(date!(2020-03-06), 9)]
#[case::dominical_ed(date!(2020-03-07), 9)]
#[case::dominical_fe(date!(2036-01-01), 0)]
#[case::dominical_fe(date!(2036-01-02), 0)]
#[case::dominical_fe(date!(2036-01-03), 0)]
#[case::dominical_fe(date!(2036-01-04), 0)]
#[case::dominical_fe(date!(2036-01-05), 0)]
#[case::dominical_fe(date!(2036-01-06), 1)]
#[case::dominical_fe(date!(2036-01-07), 1)]
#[case::dominical_fe(date!(2036-02-28), 8)]
#[case::dominical_fe(date!(2036-02-29), 8)]
#[case::dominical_fe(date!(2036-03-01), 8)]
#[case::dominical_fe(date!(2036-03-02), 9)]
#[case::dominical_fe(date!(2036-03-03), 9)]
#[case::dominical_fe(date!(2036-03-04), 9)]
#[case::dominical_fe(date!(2036-03-05), 9)]
#[case::dominical_fe(date!(2036-03-06), 9)]
#[case::dominical_fe(date!(2036-03-07), 9)]
#[case::dominical_gf(date!(2024-01-01), 0)]
#[case::dominical_gf(date!(2024-01-02), 0)]
#[case::dominical_gf(date!(2024-01-03), 0)]
#[case::dominical_gf(date!(2024-01-04), 0)]
#[case::dominical_gf(date!(2024-01-05), 0)]
#[case::dominical_gf(date!(2024-01-06), 0)]
#[case::dominical_gf(date!(2024-01-07), 1)]
#[case::dominical_gf(date!(2024-02-28), 8)]
#[case::dominical_gf(date!(2024-02-29), 8)]
#[case::dominical_gf(date!(2024-03-01), 8)]
#[case::dominical_gf(date!(2024-03-02), 8)]
#[case::dominical_gf(date!(2024-03-03), 9)]
#[case::dominical_gf(date!(2024-03-04), 9)]
#[case::dominical_gf(date!(2024-03-05), 9)]
#[case::dominical_gf(date!(2024-03-06), 9)]
#[case::dominical_gf(date!(2024-03-07), 9)]
fn sunday_based_week(#[case] date: Date, #[case] expected: u8) {
    assert_eq!(date.sunday_based_week(), expected);
}

#[rstest]
#[case(2019, 1, Monday)]
#[case(2019, 1, Tuesday)]
#[case(2020, 53, Friday)]
#[case(-9999, 1, Monday)]
fn from_iso_week_date_ok(#[case] year: i32, #[case] week: u8, #[case] weekday: Weekday) {
    assert!(Date::from_iso_week_date(year, week, weekday).is_ok());
}

#[rstest]
#[case(2019, 53, Monday)]
#[case(999999, 52, Saturday)]
fn from_iso_week_date_err(#[case] year: i32, #[case] week: u8, #[case] weekday: Weekday) {
    assert!(Date::from_iso_week_date(year, week, weekday).is_err());
}

#[rstest]
fn from_iso_week_date_regression() {
    // Regression test. Year zero (1 BCE) has dominical letter BA.
    assert_eq!(
        Date::from_iso_week_date(-1, 52, Saturday),
        Ok(date!(0000-01-01))
    );
    assert_eq!(date!(-0001-W52-6), date!(0000-01-01));
}

#[rstest]
#[case(date!(2019-002), 2019)]
#[case(date!(2020-002), 2020)]
fn year(#[case] date: Date, #[case] expected: i32) {
    assert_eq!(date.year(), expected);
}

#[rstest]
#[case(date!(2019-002), Month::January)]
#[case(date!(2020-002), Month::January)]
#[case(date!(2019-060), Month::March)]
#[case(date!(2020-060), Month::February)]
fn month(#[case] date: Date, #[case] expected: Month) {
    assert_eq!(date.month(), expected);
}

#[rstest]
#[case(date!(2019-002), 2)]
#[case(date!(2020-002), 2)]
#[case(date!(2019-060), 1)]
#[case(date!(2020-060), 29)]
fn day(#[case] date: Date, #[case] expected: u8) {
    assert_eq!(date.day(), expected);
}

#[rstest]
#[case(date!(2019-01-01), 1)]
#[case(date!(2019-10-04), 40)]
#[case(date!(2020-01-01), 1)]
#[case(date!(2020-12-31), 53)]
#[case(date!(2021-01-01), 53)]
fn iso_week(#[case] date: Date, #[case] expected: u8) {
    assert_eq!(date.iso_week(), expected);
}
#[rstest]
#[case(date!(2019-01-02), (2019, Month::January, 2))]
#[case(date!(2019-02-02), (2019, Month::February, 2))]
#[case(date!(2019-03-02), (2019, Month::March, 2))]
#[case(date!(2019-04-02), (2019, Month::April, 2))]
#[case(date!(2019-05-02), (2019, Month::May, 2))]
#[case(date!(2019-06-02), (2019, Month::June, 2))]
#[case(date!(2019-07-02), (2019, Month::July, 2))]
#[case(date!(2019-08-02), (2019, Month::August, 2))]
#[case(date!(2019-09-02), (2019, Month::September, 2))]
#[case(date!(2019-10-02), (2019, Month::October, 2))]
#[case(date!(2019-11-02), (2019, Month::November, 2))]
#[case(date!(2019-12-02), (2019, Month::December, 2))]
fn to_calendar_date(#[case] date: Date, #[case] expected: (i32, Month, u8)) {
    assert_eq!(date.to_calendar_date(), expected);
}

#[rstest]
#[case(date!(2019-01-01), (2019, 1))]
fn to_ordinal_date(#[case] date: Date, #[case] expected: (i32, u16)) {
    assert_eq!(date.to_ordinal_date(), expected);
}
#[rstest]
#[case(date!(2019-01-01), (2019, 1, Tuesday))]
#[case(date!(2019-10-04), (2019, 40, Friday))]
#[case(date!(2020-01-01), (2020, 1, Wednesday))]
#[case(date!(2020-12-31), (2020, 53, Thursday))]
#[case(date!(2021-01-01), (2020, 53, Friday))]
#[case(date!(0000-01-01), (-1, 52, Saturday))]
fn to_iso_week_date(#[case] date: Date, #[case] expected: (i32, u8, Weekday)) {
    assert_eq!(date.to_iso_week_date(), expected);
}

#[rstest]
#[case(date!(2019-01-01), Tuesday)]
#[case(date!(2019-02-01), Friday)]
#[case(date!(2019-03-01), Friday)]
#[case(date!(2019-04-01), Monday)]
#[case(date!(2019-05-01), Wednesday)]
#[case(date!(2019-06-01), Saturday)]
#[case(date!(2019-07-01), Monday)]
#[case(date!(2019-08-01), Thursday)]
#[case(date!(2019-09-01), Sunday)]
#[case(date!(2019-10-01), Tuesday)]
#[case(date!(2019-11-01), Friday)]
#[case(date!(2019-12-01), Sunday)]
fn weekday(#[case] date: Date, #[case] expected: Weekday) {
    assert_eq!(date.weekday(), expected);
}

#[rstest]
#[case(date!(2019-01-01), date!(2019-01-02))]
#[case(date!(2019-01-31), date!(2019-02-01))]
#[case(date!(2019-12-31), date!(2020-01-01))]
#[case(date!(2020-12-31), date!(2021-01-01))]
#[case(Date::MAX, None)]
fn next_day(#[case] date: Date, #[case] expected: impl Into<Option<Date>>) {
    assert_eq!(date.next_day(), expected.into());
}

#[rstest]
#[case(date!(2019-01-02), date!(2019-01-01))]
#[case(date!(2019-02-01), date!(2019-01-31))]
#[case(date!(2020-01-01), date!(2019-12-31))]
#[case(date!(2021-01-01), date!(2020-12-31))]
#[case(Date::MIN, None)]
fn previous_day(#[case] date: Date, #[case] expected: impl Into<Option<Date>>) {
    assert_eq!(date.previous_day(), expected.into());
}

#[rstest]
#[case(date!(-999_999-01-01), -363_521_074)]
#[case(date!(-9999-01-01), -1_930_999)]
#[case(date!(-4713-11-24), 0)]
#[case(date!(2000-01-01), 2_451_545)]
#[case(date!(2019-01-01), 2_458_485)]
#[case(date!(2019-12-31), 2_458_849)]
fn to_julian_day(#[case] date: Date, #[case] expected: i32) {
    assert_eq!(date.to_julian_day(), expected);
}
#[rstest]
#[case(-363_521_074, date!(-999_999-01-01))]
#[case(-1_930_999, date!(-9999-01-01))]
#[case(0, date!(-4713-11-24))]
#[case(2_451_545, date!(2000-01-01))]
#[case(2_458_485, date!(2019-01-01))]
#[case(2_458_849, date!(2019-12-31))]
#[case(i32::MAX, None)]
fn from_julian_day(#[case] julian_day: i32, #[case] expected: impl Into<Option<Date>>) {
    if let Some(expected) = expected.into() {
        assert_eq!(Date::from_julian_day(julian_day), Ok(expected));
    } else {
        assert!(Date::from_julian_day(julian_day).is_err());
    }
}

#[rstest]
#[case(date!(1970-01-01), datetime!(1970-01-01 0:00))]
#[case(date!(2023-06-15), datetime!(2023-06-15 0:00))]
#[case(date!(2000-01-01), datetime!(2000-01-01 0:00))]
fn midnight(#[case] date: Date, #[case] expected: PlainDateTime) {
    assert_eq!(date.midnight(), expected);
}

#[rstest]
#[case(date!(1970-01-01), time!(0:00), datetime!(1970-01-01 0:00))]
fn with_time(#[case] date: Date, #[case] time_value: time::Time, #[case] expected: PlainDateTime) {
    assert_eq!(date.with_time(time_value), expected);
}

#[rstest]
#[case(0, 0, 0, datetime!(1970-01-01 0:00))]
#[case(24, 0, 0, None)]
fn with_hms(
    #[case] hour: u8,
    #[case] minute: u8,
    #[case] second: u8,
    #[case] expected: impl Into<Option<PlainDateTime>>,
) {
    let result = date!(1970-01-01).with_hms(hour, minute, second);

    if let Some(expected) = expected.into() {
        assert_eq!(result, Ok(expected));
    } else {
        assert!(result.is_err());
    }
}

#[rstest]
#[case(0, 0, 0, 0, datetime!(1970-01-01 0:00))]
#[case(24, 0, 0, 0, None)]
fn with_hms_milli(
    #[case] hour: u8,
    #[case] minute: u8,
    #[case] second: u8,
    #[case] millisecond: u16,
    #[case] expected: impl Into<Option<PlainDateTime>>,
) {
    let result = date!(1970-01-01).with_hms_milli(hour, minute, second, millisecond);

    if let Some(expected) = expected.into() {
        assert_eq!(result, Ok(expected));
    } else {
        assert!(result.is_err());
    }
}

#[rstest]
#[case(0, 0, 0, 0, datetime!(1970-01-01 0:00))]
#[case(24, 0, 0, 0, None)]
fn with_hms_micro(
    #[case] hour: u8,
    #[case] minute: u8,
    #[case] second: u8,
    #[case] microsecond: u32,
    #[case] expected: impl Into<Option<PlainDateTime>>,
) {
    let result = date!(1970-01-01).with_hms_micro(hour, minute, second, microsecond);

    if let Some(expected) = expected.into() {
        assert_eq!(result, Ok(expected));
    } else {
        assert!(result.is_err());
    }
}

#[rstest]
#[case(0, 0, 0, 0, datetime!(1970-01-01 0:00))]
#[case(24, 0, 0, 0, None)]
fn with_hms_nano(
    #[case] hour: u8,
    #[case] minute: u8,
    #[case] second: u8,
    #[case] nanosecond: u32,
    #[case] expected: impl Into<Option<PlainDateTime>>,
) {
    let result = date!(1970-01-01).with_hms_nano(hour, minute, second, nanosecond);

    if let Some(expected) = expected.into() {
        assert_eq!(result, Ok(expected));
    } else {
        assert!(result.is_err());
    }
}

#[rstest]
#[case(date!(2019-01-01), 5.days(), date!(2019-01-06))]
#[case(date!(2019-12-31), 1.days(), date!(2020-01-01))]
fn add(#[case] date: Date, #[case] duration: SignedDuration, #[case] expected: Date) {
    assert_eq!(date + duration, expected);
}

#[rstest]
#[case(date!(2019-01-01), 5.std_days(), date!(2019-01-06))]
#[case(date!(2019-12-31), 1.std_days(), date!(2020-01-01))]
fn add_std(#[case] date: Date, #[case] duration: StdDuration, #[case] expected: Date) {
    assert_eq!(date + duration, expected);
}

#[rstest]
#[case(date!(2019-12-31), 1.days(), date!(2020-01-01))]
fn add_assign(#[case] date: Date, #[case] duration: SignedDuration, #[case] expected: Date) {
    let mut date = date;
    date += duration;
    assert_eq!(date, expected);
}

#[rstest]
#[case(date!(2019-12-31), 1.std_days(), date!(2020-01-01))]
fn add_assign_std(#[case] date: Date, #[case] duration: StdDuration, #[case] expected: Date) {
    let mut date = date;
    date += duration;
    assert_eq!(date, expected);
}

#[rstest]
#[case(date!(2019-01-06), 5.days(), date!(2019-01-01))]
#[case(date!(2020-01-01), 1.days(), date!(2019-12-31))]
fn sub(#[case] date: Date, #[case] duration: SignedDuration, #[case] expected: Date) {
    assert_eq!(date - duration, expected);
}

#[rstest]
#[case(date!(2019-01-06), 5.std_days(), date!(2019-01-01))]
#[case(date!(2020-01-01), 1.std_days(), date!(2019-12-31))]
fn sub_std(#[case] date: Date, #[case] duration: StdDuration, #[case] expected: Date) {
    assert_eq!(date - duration, expected);
}

#[rstest]
#[case(date!(2020-01-01), 1.days(), date!(2019-12-31))]
fn sub_assign(#[case] date: Date, #[case] duration: SignedDuration, #[case] expected: Date) {
    let mut date = date;
    date -= duration;
    assert_eq!(date, expected);
}

#[rstest]
#[case(date!(2020-01-01), 1.std_days(), date!(2019-12-31))]
fn sub_assign_std(#[case] date: Date, #[case] duration: StdDuration, #[case] expected: Date) {
    let mut date = date;
    date -= duration;
    assert_eq!(date, expected);
}

#[rstest]
#[case(date!(2019-01-06), date!(2019-01-01), 5.days())]
#[case(date!(2020-01-01), date!(2019-12-31), 1.days())]
fn sub_self(#[case] a: Date, #[case] b: Date, #[case] expected: SignedDuration) {
    assert_eq!(a - b, expected);
}

#[rstest]
#[case(date!(2019-01-01), date!(2019-01-02))]
fn partial_ord(#[case] first: Date, #[case] second: Date) {
    assert_eq!(first.partial_cmp(&first), Some(Ordering::Equal));
    assert_eq!(first.partial_cmp(&second), Some(Ordering::Less));
    assert_eq!(second.partial_cmp(&first), Some(Ordering::Greater));
}

#[rstest]
#[case(date!(2019-01-01), date!(2019-01-02))]
fn ord(#[case] first: Date, #[case] second: Date) {
    assert_eq!(first.cmp(&first), Ordering::Equal);
    assert_eq!(first.cmp(&second), Ordering::Less);
    assert_eq!(second.cmp(&first), Ordering::Greater);
}

#[rstest]
fn regression_check() {
    let (year, week, weekday) = (date!(0063-365)).to_iso_week_date();
    assert_eq!(year, 64);
    assert_eq!(week, 1);
    assert_eq!(weekday, Monday);
}

#[rstest]
#[case(Date::MIN, SignedDuration::new(86_399, 999_999_999), Date::MIN)]
#[case(Date::MIN, SignedDuration::new(-86_399, -999_999_999), Date::MIN)]
#[case(date!(2021-10-25), SignedDuration::new(86_399, 999_999_999), date!(2021-10-25))]
#[case(date!(2021-10-25), SignedDuration::new(-86_399, -999_999_999), date!(2021-10-25))]
#[case(Date::MAX, SignedDuration::new(86_399, 999_999_999), Date::MAX)]
#[case(Date::MAX, SignedDuration::new(-86_399, -999_999_999), Date::MAX)]
#[case(Date::MIN, SignedDuration::DAY, Date::MIN.next_day())]
#[case(Date::MIN, -SignedDuration::DAY, None)]
#[case(date!(2021-10-25), SignedDuration::DAY, date!(2021-10-26))]
#[case(date!(2021-10-25), -SignedDuration::DAY, date!(2021-10-24))]
#[case(Date::MAX, SignedDuration::DAY, None)]
#[case(Date::MAX, -SignedDuration::DAY, Date::MAX.previous_day())]
#[case(Date::MIN, SignedDuration::MIN, None)]
#[case(Date::MAX, SignedDuration::MAX, None)]
fn checked_add_duration(
    #[case] date: Date,
    #[case] duration: SignedDuration,
    #[case] expected: impl Into<Option<Date>>,
) {
    assert_eq!(date.checked_add(duration), expected.into());
}

#[rstest]
#[case(Date::MIN, SignedDuration::new(86_399, 999_999_999), Date::MIN)]
#[case(Date::MIN, SignedDuration::new(-86_399, -999_999_999), Date::MIN)]
#[case(date!(2021-10-25), SignedDuration::new(86_399, 999_999_999), date!(2021-10-25))]
#[case(date!(2021-10-25), SignedDuration::new(-86_399, -999_999_999), date!(2021-10-25))]
#[case(Date::MAX, SignedDuration::new(86_399, 999_999_999), Date::MAX)]
#[case(Date::MAX, SignedDuration::new(-86_399, -999_999_999), Date::MAX)]
#[case(Date::MIN, SignedDuration::DAY, None)]
#[case(Date::MIN, -SignedDuration::DAY, Date::MIN.next_day())]
#[case(date!(2021-10-25), SignedDuration::DAY, date!(2021-10-24))]
#[case(date!(2021-10-25), -SignedDuration::DAY, date!(2021-10-26))]
#[case(Date::MAX, SignedDuration::DAY, Date::MAX.previous_day())]
#[case(Date::MAX, -SignedDuration::DAY, None)]
#[case(Date::MIN, SignedDuration::MAX, None)]
#[case(Date::MAX, SignedDuration::MIN, None)]
fn checked_sub_duration(
    #[case] date: Date,
    #[case] duration: SignedDuration,
    #[case] expected: impl Into<Option<Date>>,
) {
    assert_eq!(date.checked_sub(duration), expected.into());
}

#[rstest]
#[case(date!(2021-11-05), 2.days(), date!(2021-11-07))]
#[case(date!(2021-11-05), (-2).days(), date!(2021-11-03))]
#[case(Date::MIN, (-10).days(), Date::MIN)]
#[case(Date::MAX, 10.days(), Date::MAX)]
#[case(Date::MIN, SignedDuration::ZERO, Date::MIN)]
#[case(Date::MAX, SignedDuration::ZERO, Date::MAX)]
fn saturating_add_duration(
    #[case] date: Date,
    #[case] duration: SignedDuration,
    #[case] expected: Date,
) {
    assert_eq!(date.saturating_add(duration), expected);
}

#[rstest]
#[case(date!(2021-11-05), 2.days(), date!(2021-11-03))]
#[case(date!(2021-11-05), (-2).days(), date!(2021-11-07))]
#[case(Date::MIN, 10.days(), Date::MIN)]
#[case(Date::MAX, (-10).days(), Date::MAX)]
#[case(Date::MIN, SignedDuration::ZERO, Date::MIN)]
#[case(Date::MAX, SignedDuration::ZERO, Date::MAX)]
fn saturating_sub_duration(
    #[case] date: Date,
    #[case] duration: SignedDuration,
    #[case] expected: Date,
) {
    assert_eq!(date.saturating_sub(duration), expected);
}

#[rstest]
#[case(date!(2022-02-18), 2019, date!(2019-02-18))]
#[case(date!(2022-02-18), -1_000_000_000, None)]
#[case(date!(2022-02-18), 1_000_000_000, None)]
#[case(date!(2022-01-01), 2024, date!(2024-01-01))]
#[case(date!(2022-12-01), 2024, date!(2024-12-01))]
#[case(date!(2024-01-01), 2022, date!(2022-01-01))]
#[case(date!(2024-12-01), 2022, date!(2022-12-01))]
#[case(date!(2024-02-29), 2022, None)]
#[case(date!(2022-12-01), 2023, date!(2023-12-01))]
#[case(date!(2024-12-01), 2028, date!(2028-12-01))]
fn replace_year(#[case] date: Date, #[case] year: i32, #[case] expected: impl Into<Option<Date>>) {
    if let Some(expected) = expected.into() {
        assert_eq!(date.replace_year(year), Ok(expected));
    } else {
        assert!(date.replace_year(year).is_err());
    }
}

#[rstest]
#[case(date!(2022-02-18), Month::January, date!(2022-01-18))]
#[case(date!(2022-01-30), Month::February, None)]
fn replace_month(
    #[case] date: Date,
    #[case] month: Month,
    #[case] expected: impl Into<Option<Date>>,
) {
    if let Some(expected) = expected.into() {
        assert_eq!(date.replace_month(month), Ok(expected));
    } else {
        assert!(date.replace_month(month).is_err());
    }
}

#[rstest]
#[case(date!(2022-02-18), 1, date!(2022-02-01))]
#[case(date!(2022-02-18), 0, None)]
#[case(date!(2022-02-18), 30, None)]
fn replace_day(#[case] date: Date, #[case] day: u8, #[case] expected: impl Into<Option<Date>>) {
    if let Some(expected) = expected.into() {
        assert_eq!(date.replace_day(day), Ok(expected));
    } else {
        assert!(date.replace_day(day).is_err());
    }
}

#[rstest]
#[case(date!(2022-02-18), 1, date!(2022-001))]
#[case(date!(2024-02-29), 366, date!(2024-366))]
#[case(date!(2022-049), 0, None)]
#[case(date!(2022-049), 366, None)]
#[case(date!(2022-049), 367, None)]
fn replace_ordinal(
    #[case] date: Date,
    #[case] ordinal: u16,
    #[case] expected: impl Into<Option<Date>>,
) {
    if let Some(expected) = expected.into() {
        assert_eq!(date.replace_ordinal(ordinal), Ok(expected));
    } else {
        assert!(date.replace_ordinal(ordinal).is_err());
    }
}

#[rstest]
#[case(date!(2023-06-25), Monday, date!(2023-06-26))]
#[case(date!(2023-06-26), Monday, date!(2023-07-03))]
#[case(date!(2023-06-27), Monday, date!(2023-07-03))]
#[case(date!(2023-06-28), Monday, date!(2023-07-03))]
#[case(date!(2023-06-29), Monday, date!(2023-07-03))]
#[case(date!(2023-06-30), Monday, date!(2023-07-03))]
#[case(date!(2023-07-01), Monday, date!(2023-07-03))]
#[case(date!(2023-07-02), Monday, date!(2023-07-03))]
#[case(date!(2023-07-03), Monday, date!(2023-07-10))]
fn next_occurrence_test(#[case] date: Date, #[case] weekday: Weekday, #[case] expected: Date) {
    assert_eq!(date.next_occurrence(weekday), expected);
}

#[rstest]
#[case(date!(2023-07-07), Thursday, date!(2023-07-06))]
#[case(date!(2023-07-06), Thursday, date!(2023-06-29))]
#[case(date!(2023-07-05), Thursday, date!(2023-06-29))]
#[case(date!(2023-07-04), Thursday, date!(2023-06-29))]
#[case(date!(2023-07-03), Thursday, date!(2023-06-29))]
#[case(date!(2023-07-02), Thursday, date!(2023-06-29))]
#[case(date!(2023-07-01), Thursday, date!(2023-06-29))]
#[case(date!(2023-06-30), Thursday, date!(2023-06-29))]
#[case(date!(2023-06-29), Thursday, date!(2023-06-22))]
fn prev_occurrence_test(#[case] date: Date, #[case] weekday: Weekday, #[case] expected: Date) {
    assert_eq!(date.prev_occurrence(weekday), expected);
}

#[rstest]
#[case(date!(2023-06-25), Monday, 5, date!(2023-07-24))]
#[case(date!(2023-06-26), Monday, 5, date!(2023-07-31))]
fn nth_next_occurrence_test(
    #[case] date: Date,
    #[case] weekday: Weekday,
    #[case] n: u8,
    #[case] expected: Date,
) {
    assert_eq!(date.nth_next_occurrence(weekday, n), expected);
}

#[rstest]
#[case(date!(2023-06-27), Monday, 3, date!(2023-06-12))]
#[case(date!(2023-06-26), Monday, 3, date!(2023-06-05))]
fn nth_prev_occurrence_test(
    #[case] date: Date,
    #[case] weekday: Weekday,
    #[case] n: u8,
    #[case] expected: Date,
) {
    assert_eq!(date.nth_prev_occurrence(weekday, n), expected);
}

#[rstest]
#[should_panic]
fn next_occurrence_overflow_test() {
    date!(+999999-12-25).next_occurrence(Saturday);
}

#[rstest]
#[should_panic]
fn prev_occurrence_overflow_test() {
    date!(-999999-01-07).prev_occurrence(Sunday);
}

#[rstest]
#[should_panic]
fn nth_next_occurrence_overflow_test() {
    date!(+999999-12-25).nth_next_occurrence(Saturday, 1);
}

#[rstest]
#[should_panic]
fn nth_next_occurence_zeroth_occurence_test() {
    date!(2023-06-25).nth_next_occurrence(Monday, 0);
}

#[rstest]
#[should_panic]
fn nth_prev_occurence_zeroth_occurence_test() {
    date!(2023-06-27).nth_prev_occurrence(Monday, 0);
}

#[rstest]
#[should_panic]
fn nth_prev_occurrence_overflow_test() {
    date!(-999999-01-07).nth_prev_occurrence(Sunday, 1);
}
