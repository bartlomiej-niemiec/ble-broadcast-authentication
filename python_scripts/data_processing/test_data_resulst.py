from dataclasses import dataclass


@dataclass
class TestData:
    sender_advertising_interval_ms: int
    sender_total_pdus_send: int
    sender_payload_size: int
    packets_received: int
    packets_malformed: int
    average_kr_tim_in_s: float
    average_dq_fill_for_kr: float

    def get_packet_loss(self):
        return ((self.sender_total_pdus_send - self.packets_received) / self.sender_total_pdus_send) * 100.0


@dataclass
class AverageQueueFill:
    random_key_fragment_selection: float
    sequence_key_fragment_selection: float
    adv_interval_ms: int


class DataCollection:

    def __init__(self, payload_size):
        self.payload_size = payload_size
        self.results = []

    def add_results(self, result):
        self.results.append(result)

    def get_results_arr(self):
        return self.results


TEST_DATA_4_BYTES = [
    TestData(
        sender_advertising_interval_ms=20,
        sender_total_pdus_send=2000,
        sender_payload_size=4,
        packets_received=1552,
        packets_malformed=0,
        average_kr_tim_in_s=0.14,
        average_dq_fill_for_kr=11.38
    ),
    TestData(
        sender_advertising_interval_ms=50,
        sender_total_pdus_send=2000,
        sender_payload_size=4,
        packets_received=1757,
        packets_malformed=0,
        average_kr_tim_in_s=0.43,
        average_dq_fill_for_kr=11.25
    ),
    TestData(
        sender_advertising_interval_ms=100,
        sender_total_pdus_send=2000,
        sender_payload_size=4,
        packets_received=1840,
        packets_malformed=0,
        average_kr_tim_in_s=0.69,
        average_dq_fill_for_kr=9.50
    ),
    TestData(
        sender_advertising_interval_ms=300,
        sender_total_pdus_send=2000,
        sender_payload_size=4,
        packets_received=1878,
        packets_malformed=0,
        average_kr_tim_in_s=2.44,
        average_dq_fill_for_kr=10.00
    ),
    TestData(
        sender_advertising_interval_ms=500,
        sender_total_pdus_send=2000,
        sender_payload_size=4,
        packets_received=1914,
        packets_malformed=0,
        average_kr_tim_in_s=4.33,
        average_dq_fill_for_kr=11.00
    ),
    TestData(
        sender_advertising_interval_ms=1000,
        sender_total_pdus_send=2000,
        sender_payload_size=4,
        packets_received=1894,
        packets_malformed=0,
        average_kr_tim_in_s=8.56,
        average_dq_fill_for_kr=11.25
    )
]

TEST_DATA_10_BYTES = [
    TestData(
        sender_advertising_interval_ms=20,
        sender_total_pdus_send=2000,
        sender_payload_size=10,
        packets_received=1539,
        packets_malformed=0,
        average_kr_tim_in_s=0.26,
        average_dq_fill_for_kr=15.25
    ),
    TestData(
        sender_advertising_interval_ms=50,
        sender_total_pdus_send=2000,
        sender_payload_size=10,
        packets_received=1753,
        packets_malformed=0,
        average_kr_tim_in_s=0.36,
        average_dq_fill_for_kr=9.62
    ),
    TestData(
        sender_advertising_interval_ms=100,
        sender_total_pdus_send=2000,
        sender_payload_size=10,
        packets_received=1834,
        packets_malformed=0,
        average_kr_tim_in_s=0.72,
        average_dq_fill_for_kr=9.13
    ),
    TestData(
        sender_advertising_interval_ms=300,
        sender_total_pdus_send=2000,
        sender_payload_size=10,
        packets_received=1896,
        packets_malformed=0,
        average_kr_tim_in_s=2.04,
        average_dq_fill_for_kr=9.25
    ),
    TestData(
        sender_advertising_interval_ms=500,
        sender_total_pdus_send=2000,
        sender_payload_size=10,
        packets_received=1918,
        packets_malformed=0,
        average_kr_tim_in_s=3.60,
        average_dq_fill_for_kr=9.75
    ),
    TestData(
        sender_advertising_interval_ms=1000,
        sender_total_pdus_send=2000,
        sender_payload_size=10,
        packets_received=1906,
        packets_malformed=0,
        average_kr_tim_in_s=7.94,
        average_dq_fill_for_kr=10.75
    )
]

TEST_DATA_16_BYTES = [
    TestData(
        sender_advertising_interval_ms=20,
        sender_total_pdus_send=2000,
        sender_payload_size=4,
        packets_received=1527,
        packets_malformed=0,
        average_kr_tim_in_s=0.26,
        average_dq_fill_for_kr=16.00
    ),
    TestData(
        sender_advertising_interval_ms=50,
        sender_total_pdus_send=2000,
        sender_payload_size=16,
        packets_received=1749,
        packets_malformed=0,
        average_kr_tim_in_s=0.39,
        average_dq_fill_for_kr=10.62
    ),
    TestData(
        sender_advertising_interval_ms=100,
        sender_total_pdus_send=2000,
        sender_payload_size=16,
        packets_received=1831,
        packets_malformed=0,
        average_kr_tim_in_s=0.85,
        average_dq_fill_for_kr=11.00
    ),
    TestData(
        sender_advertising_interval_ms=300,
        sender_total_pdus_send=2000,
        sender_payload_size=16,
        packets_received=1905,
        packets_malformed=0,
        average_kr_tim_in_s=2.51,
        average_dq_fill_for_kr=10.87
    ),
    TestData(
        sender_advertising_interval_ms=500,
        sender_total_pdus_send=2000,
        sender_payload_size=16,
        packets_received=1907,
        packets_malformed=0,
        average_kr_tim_in_s=4.20,
        average_dq_fill_for_kr=11.25
    ),
    TestData(
        sender_advertising_interval_ms=1000,
        sender_total_pdus_send=2000,
        sender_payload_size=16,
        packets_received=1898,
        packets_malformed=0,
        average_kr_tim_in_s=7.95,
        average_dq_fill_for_kr=10.62
    )
]


@dataclass
class DynamicTestData:
    sender_advertising_interval_min: int
    sender_advertising_interval_max: int
    sender_advertising_interval_scaler: int
    sender_total_pdus_send: int
    is_payload_size_random: bool
    sender_payload_size: int
    packets_received: int
    packets_malformed: int
    average_kr_tim_in_s: float
    average_dq_fill_for_kr: float

    def get_packet_loss(self):
        return ((self.sender_total_pdus_send - self.packets_received) / self.sender_total_pdus_send) * 100.0


DYNAMIC_TESTS_RESULTS = [
    DynamicTestData(
        sender_advertising_interval_min=20,
        sender_advertising_interval_max=200,
        sender_advertising_interval_scaler=20,
        sender_total_pdus_send=2000,
        is_payload_size_random=True,
        sender_payload_size=16,
        packets_received=1789,
        packets_malformed=0,
        average_kr_tim_in_s=0.56,
        average_dq_fill_for_kr=11.77
    ),
    DynamicTestData(
        sender_advertising_interval_min=200,
        sender_advertising_interval_max=1000,
        sender_advertising_interval_scaler=100,
        sender_total_pdus_send=2000,
        sender_payload_size=16,
        is_payload_size_random=True,
        packets_received=1872,
        packets_malformed=0,
        average_kr_tim_in_s=4.85,
        average_dq_fill_for_kr=10.12
    ),
    DynamicTestData(
        sender_advertising_interval_min=1000,
        sender_advertising_interval_max=3000,
        sender_advertising_interval_scaler=250,
        sender_total_pdus_send=2000,
        is_payload_size_random=True,
        sender_payload_size=16,
        packets_received=1902,
        packets_malformed=0,
        average_kr_tim_in_s=11.08,
        average_dq_fill_for_kr=10.12
    ),
    DynamicTestData(
        sender_advertising_interval_min=3000,
        sender_advertising_interval_max=5000,
        sender_advertising_interval_scaler=250,
        sender_total_pdus_send=2000,
        is_payload_size_random=True,
        sender_payload_size=16,
        packets_received=1872,
        packets_malformed=0,
        average_kr_tim_in_s=28.32,
        average_dq_fill_for_kr=10.00
    )
]

MULTIPLE_SENDERS_20MS_BOTH = (
    TestData(
        sender_advertising_interval_ms=20,
        sender_total_pdus_send=2000,
        sender_payload_size=10,
        packets_received=1485,
        packets_malformed=0,
        average_kr_tim_in_s=0.20,
        average_dq_fill_for_kr=13.75
    ),
    TestData(
        sender_advertising_interval_ms=20,
        sender_total_pdus_send=2000,
        sender_payload_size=10,
        packets_received=1462,
        packets_malformed=0,
        average_kr_tim_in_s=0.26,
        average_dq_fill_for_kr=15.38
    )
)

MULTIPLE_SENDERS_50MS_BOTH = (
    TestData(
        sender_advertising_interval_ms=50,
        sender_total_pdus_send=2000,
        sender_payload_size=10,
        packets_received=1695,
        packets_malformed=0,
        average_kr_tim_in_s=0.42,
        average_dq_fill_for_kr=10.50
    ),
    TestData(
        sender_advertising_interval_ms=50,
        sender_total_pdus_send=2000,
        sender_payload_size=10,
        packets_received=1677,
        packets_malformed=0,
        average_kr_tim_in_s=0.51,
        average_dq_fill_for_kr=13.12
    )
)

MULTIPLE_SENDERS_100MS_BOTH = (
    TestData(
        sender_advertising_interval_ms=100,
        sender_total_pdus_send=2000,
        sender_payload_size=10,
        packets_received=1805,
        packets_malformed=0,
        average_kr_tim_in_s=0.77,
        average_dq_fill_for_kr=9.87
    ),
    TestData(
        sender_advertising_interval_ms=100,
        sender_total_pdus_send=2000,
        sender_payload_size=10,
        packets_received=1802,
        packets_malformed=0,
        average_kr_tim_in_s=0.94,
        average_dq_fill_for_kr=10.87
    )
)

MULTIPLE_SENDERS_300MS_BOTH = (
    TestData(
        sender_advertising_interval_ms=300,
        sender_total_pdus_send=2000,
        sender_payload_size=10,
        packets_received=1867,
        packets_malformed=0,
        average_kr_tim_in_s=2.63,
        average_dq_fill_for_kr=11.25
    ),
    TestData(
        sender_advertising_interval_ms=300,
        sender_total_pdus_send=2000,
        sender_payload_size=10,
        packets_received=1690,
        packets_malformed=187,
        average_kr_tim_in_s=2.81,
        average_dq_fill_for_kr=12.08
    )
)

MULTIPLE_SENDERS_500MS_BOTH = (
    TestData(
        sender_advertising_interval_ms=500,
        sender_total_pdus_send=2000,
        sender_payload_size=10,
        packets_received=1903,
        packets_malformed=0,
        average_kr_tim_in_s=3.24,
        average_dq_fill_for_kr=8.88
    ),
    TestData(
        sender_advertising_interval_ms=500,
        sender_total_pdus_send=2000,
        sender_payload_size=10,
        packets_received=1708,
        packets_malformed=189,
        average_kr_tim_in_s=3.78,
        average_dq_fill_for_kr=10.14
    )
)

MULTIPLE_SENDERS_1000MS_BOTH = (
    TestData(
        sender_advertising_interval_ms=1000,
        sender_total_pdus_send=2000,
        sender_payload_size=10,
        packets_received=1707,
        packets_malformed=194,
        average_kr_tim_in_s=7.45,
        average_dq_fill_for_kr=9.38
    ),
    TestData(
        sender_advertising_interval_ms=1000,
        sender_total_pdus_send=2000,
        sender_payload_size=10,
        packets_received=1897,
        packets_malformed=0,
        average_kr_tim_in_s=5.40,
        average_dq_fill_for_kr=7.75
    )
)

MULTIPLE_SENDERS_RESULTS = [
    MULTIPLE_SENDERS_20MS_BOTH,
    MULTIPLE_SENDERS_50MS_BOTH,
    MULTIPLE_SENDERS_100MS_BOTH,
    MULTIPLE_SENDERS_300MS_BOTH,
    MULTIPLE_SENDERS_500MS_BOTH,
    MULTIPLE_SENDERS_1000MS_BOTH
]

AVERAGE_QUEUE_FILL_DATA = [
    AverageQueueFill(
        random_key_fragment_selection=15.25,
        sequence_key_fragment_selection=13.12,
        adv_interval_ms=20
    ),
    AverageQueueFill(
        random_key_fragment_selection=9.62,
        sequence_key_fragment_selection=7.38,
        adv_interval_ms=50
    ),
    AverageQueueFill(
        random_key_fragment_selection=9.13,
        sequence_key_fragment_selection=5.63,
        adv_interval_ms=100
    ),
    AverageQueueFill(
        random_key_fragment_selection=9.25,
        sequence_key_fragment_selection=5.25,
        adv_interval_ms=300
    ),
    AverageQueueFill(
        random_key_fragment_selection=9.75,
        sequence_key_fragment_selection=5.00,
        adv_interval_ms=500
    ),
    AverageQueueFill(
        random_key_fragment_selection=10.75,
        sequence_key_fragment_selection=5.13,
        adv_interval_ms=1000
    ),
]

MULTIPLE_SENDERS_DYNAMICS = [(
    DynamicTestData(
        sender_advertising_interval_min=20,
        sender_advertising_interval_max=200,
        sender_advertising_interval_scaler=20,
        sender_total_pdus_send=2000,
        is_payload_size_random=True,
        sender_payload_size=10,
        packets_received=1694,
        packets_malformed=0,
        average_kr_tim_in_s=0.56,
        average_dq_fill_for_kr=11.77
    ),
    DynamicTestData(
        sender_advertising_interval_min=20,
        sender_advertising_interval_max=200,
        sender_advertising_interval_scaler=20,
        sender_total_pdus_send=2000,
        is_payload_size_random=True,
        sender_payload_size=10,
        packets_received=1756,
        packets_malformed=0,
        average_kr_tim_in_s=0.56,
        average_dq_fill_for_kr=11.77
    )
),
(
    DynamicTestData(
        sender_advertising_interval_min=200,
        sender_advertising_interval_max=1000,
        sender_advertising_interval_scaler=100,
        sender_total_pdus_send=2000,
        sender_payload_size=10,
        is_payload_size_random=True,
        packets_received=1899,
        packets_malformed=0,
        average_kr_tim_in_s=4.85,
        average_dq_fill_for_kr=10.12
    ),
    DynamicTestData(
        sender_advertising_interval_min=200,
        sender_advertising_interval_max=1000,
        sender_advertising_interval_scaler=100,
        sender_total_pdus_send=2000,
        sender_payload_size=10,
        is_payload_size_random=True,
        packets_received=1894,
        packets_malformed=0,
        average_kr_tim_in_s=4.85,
        average_dq_fill_for_kr=10.12
    )
),
(
    DynamicTestData(
        sender_advertising_interval_min=1000,
        sender_advertising_interval_max=3000,
        sender_advertising_interval_scaler=250,
        sender_total_pdus_send=2000,
        is_payload_size_random=True,
        sender_payload_size=10,
        packets_received=1720,
        packets_malformed=196,
        average_kr_tim_in_s=11.08,
        average_dq_fill_for_kr=10.12
    ),
    DynamicTestData(
        sender_advertising_interval_min=1000,
        sender_advertising_interval_max=3000,
        sender_advertising_interval_scaler=250,
        sender_total_pdus_send=2000,
        is_payload_size_random=True,
        sender_payload_size=10,
        packets_received=1898,
        packets_malformed=0,
        average_kr_tim_in_s=11.08,
        average_dq_fill_for_kr=10.12
    )
),
(
    DynamicTestData(
        sender_advertising_interval_min=3000,
        sender_advertising_interval_max=5000,
        sender_advertising_interval_scaler=250,
        sender_total_pdus_send=2000,
        is_payload_size_random=True,
        sender_payload_size=10,
        packets_received=1902,
        packets_malformed=0,
        average_kr_tim_in_s=28.32,
        average_dq_fill_for_kr=10.00
    ),
    DynamicTestData(
        sender_advertising_interval_min=3000,
        sender_advertising_interval_max=5000,
        sender_advertising_interval_scaler=250,
        sender_total_pdus_send=2000,
        is_payload_size_random=True,
        sender_payload_size=10,
        packets_received=1907,
        packets_malformed=0,
        average_kr_tim_in_s=28.32,
        average_dq_fill_for_kr=10.00
    )
)
]

IMPROVED_ALGORITHM_DYNAMIC_TESTS_RESULTS = [
    DynamicTestData(
        sender_advertising_interval_min=20,
        sender_advertising_interval_max=200,
        sender_advertising_interval_scaler=20,
        sender_total_pdus_send=2000,
        is_payload_size_random=True,
        sender_payload_size=10,
        packets_received=1103,
        packets_malformed=0,
        average_kr_tim_in_s=3.95,
        average_dq_fill_for_kr=25.14
    ),
    DynamicTestData(
        sender_advertising_interval_min=200,
        sender_advertising_interval_max=1000,
        sender_advertising_interval_scaler=80,
        sender_total_pdus_send=2000,
        sender_payload_size=10,
        is_payload_size_random=True,
        packets_received=1753,
        packets_malformed=0,
        average_kr_tim_in_s=9.20,
        average_dq_fill_for_kr=20.00
    ),
    DynamicTestData(
        sender_advertising_interval_min=1000,
        sender_advertising_interval_max=3000,
        sender_advertising_interval_scaler=80,
        sender_total_pdus_send=2000,
        is_payload_size_random=True,
        sender_payload_size=10,
        packets_received=1902,
        packets_malformed=0,
        average_kr_tim_in_s=26.64,
        average_dq_fill_for_kr=13.00
    ),
    DynamicTestData(
        sender_advertising_interval_min=3000,
        sender_advertising_interval_max=5000,
        sender_advertising_interval_scaler=80,
        sender_total_pdus_send=2000,
        is_payload_size_random=True,
        sender_payload_size=10,
        packets_received=1910,
        packets_malformed=0,
        average_kr_tim_in_s=59.80,
        average_dq_fill_for_kr=16.50
    )
]

IMPROVED_ALGORITHM_MULTIPLE_SENDERS_DYNAMICS = [(
    DynamicTestData(
        sender_advertising_interval_min=20,
        sender_advertising_interval_max=200,
        sender_advertising_interval_scaler=20,
        sender_total_pdus_send=2000,
        is_payload_size_random=True,
        sender_payload_size=10,
        packets_received=1058,
        packets_malformed=0,
        average_kr_tim_in_s=2.96,
        average_dq_fill_for_kr=28.12
    ),
    DynamicTestData(
        sender_advertising_interval_min=20,
        sender_advertising_interval_max=200,
        sender_advertising_interval_scaler=20,
        sender_total_pdus_send=2000,
        is_payload_size_random=True,
        sender_payload_size=10,
        packets_received=1345,
        packets_malformed=0,
        average_kr_tim_in_s=2.31,
        average_dq_fill_for_kr=15.28
    )
),
(
    DynamicTestData(
        sender_advertising_interval_min=200,
        sender_advertising_interval_max=1000,
        sender_advertising_interval_scaler=100,
        sender_total_pdus_send=2000,
        sender_payload_size=10,
        is_payload_size_random=True,
        packets_received=1790,
        packets_malformed=0,
        average_kr_tim_in_s=8.32,
        average_dq_fill_for_kr=17.25
    ),
    DynamicTestData(
        sender_advertising_interval_min=200,
        sender_advertising_interval_max=1000,
        sender_advertising_interval_scaler=100,
        sender_total_pdus_send=2000,
        sender_payload_size=10,
        is_payload_size_random=True,
        packets_received=1744,
        packets_malformed=0,
        average_kr_tim_in_s=9.70,
        average_dq_fill_for_kr=18.00
    )
),
(
    DynamicTestData(
        sender_advertising_interval_min=1000,
        sender_advertising_interval_max=3000,
        sender_advertising_interval_scaler=250,
        sender_total_pdus_send=2000,
        is_payload_size_random=True,
        sender_payload_size=10,
        packets_received=1889,
        packets_malformed=0,
        average_kr_tim_in_s=22.63,
        average_dq_fill_for_kr=15.50
    ),
    DynamicTestData(
        sender_advertising_interval_min=1000,
        sender_advertising_interval_max=3000,
        sender_advertising_interval_scaler=250,
        sender_total_pdus_send=2000,
        is_payload_size_random=True,
        sender_payload_size=10,
        packets_received=1898,
        packets_malformed=0,
        average_kr_tim_in_s=31.65,
        average_dq_fill_for_kr=16.50
    )
),
(
    DynamicTestData(
        sender_advertising_interval_min=3000,
        sender_advertising_interval_max=5000,
        sender_advertising_interval_scaler=250,
        sender_total_pdus_send=2000,
        is_payload_size_random=True,
        sender_payload_size=10,
        packets_received=2000,
        packets_malformed=0,
        average_kr_tim_in_s=28.32,
        average_dq_fill_for_kr=10.00
    ),
    DynamicTestData(
        sender_advertising_interval_min=3000,
        sender_advertising_interval_max=5000,
        sender_advertising_interval_scaler=250,
        sender_total_pdus_send=2000,
        is_payload_size_random=True,
        sender_payload_size=10,
        packets_received=2000,
        packets_malformed=0,
        average_kr_tim_in_s=28.32,
        average_dq_fill_for_kr=10.00
    )
)
]
