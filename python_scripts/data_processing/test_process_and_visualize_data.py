import matplotlib.pyplot as plt
import numpy as np
import python_scripts.data_processing.test_data_resulst as results
from prettytable import PrettyTable
import statistics

TESTED_PAYLOAD_SIZES_IN_BYTES = [4, 10, 16]
TESTED_ADVERTISING_INTERVALS_IN_MS = [20, 50, 100, 300, 500, 1000]
TESTED_NO_SEND_PACKETS = [2000]


def get_packet_losses():
    # GET PACKET LOSSES FOR 4 BYTES DATA
    PACKET_LOSS_4_BYTES = []
    for data in results.TEST_DATA_4_BYTES:
        PACKET_LOSS_4_BYTES.append(
            data.get_packet_loss()
        )

    # GET PACKET LOSSES FOR 10 BYTES DATA
    PACKET_LOSS_10_BYTES = []
    for data in results.TEST_DATA_10_BYTES:
        PACKET_LOSS_10_BYTES.append(
            data.get_packet_loss()
        )

    # GET PACKET LOSSES FOR 16 BYTES DATA
    PACKET_LOSS_16_BYTES = []
    for data in results.TEST_DATA_16_BYTES:
        PACKET_LOSS_16_BYTES.append(
            data.get_packet_loss()
        )

    return (
        PACKET_LOSS_4_BYTES,
        PACKET_LOSS_10_BYTES,
        PACKET_LOSS_16_BYTES
    )


def get_average_reconstruction_time_for_interval(interval):
    AVERAGE_RECONSTRUCTION_TIME_FOR_DIFFERENT_PAYLOADS = []

    for data in results.TEST_DATA_4_BYTES:
        if data.sender_advertising_interval_ms == interval:
            AVERAGE_RECONSTRUCTION_TIME_FOR_DIFFERENT_PAYLOADS.append(
                data.average_kr_tim_in_s
            )

    for data in results.TEST_DATA_10_BYTES:
        if data.sender_advertising_interval_ms == interval:
            AVERAGE_RECONSTRUCTION_TIME_FOR_DIFFERENT_PAYLOADS.append(
                data.average_kr_tim_in_s
            )

    for data in results.TEST_DATA_16_BYTES:
        if data.sender_advertising_interval_ms == interval:
            AVERAGE_RECONSTRUCTION_TIME_FOR_DIFFERENT_PAYLOADS.append(
                data.average_kr_tim_in_s
            )

    return AVERAGE_RECONSTRUCTION_TIME_FOR_DIFFERENT_PAYLOADS


def get_average_dq_fill_for_interval(interval):
    AVERAGE_DQ_FILL_DIFFERENT_PAYLOADS = []

    for data in results.TEST_DATA_4_BYTES:
        if data.sender_advertising_interval_ms == interval:
            AVERAGE_DQ_FILL_DIFFERENT_PAYLOADS.append(
                data.average_dq_fill_for_kr
            )

    for data in results.TEST_DATA_10_BYTES:
        if data.sender_advertising_interval_ms == interval:
            AVERAGE_DQ_FILL_DIFFERENT_PAYLOADS.append(
                data.average_dq_fill_for_kr
            )

    for data in results.TEST_DATA_16_BYTES:
        if data.sender_advertising_interval_ms == interval:
            AVERAGE_DQ_FILL_DIFFERENT_PAYLOADS.append(
                data.average_dq_fill_for_kr
            )

    return AVERAGE_DQ_FILL_DIFFERENT_PAYLOADS


def plot_packet_losses(packet_loss_4bytes, packet_loss_10bytes, packet_loss_16bytes):
    x_points_adv_intervals = np.array(TESTED_ADVERTISING_INTERVALS_IN_MS)

    # 4 BYTES PLOT
    y_points_4_bytes = np.array(packet_loss_4bytes)
    plt.scatter(x_points_adv_intervals, y_points_4_bytes, color='r', marker="+", linewidth=5, label='4 bajty danych')

    # 10 BYTES PLOT
    y_points_10_bytes = np.array(packet_loss_10bytes)
    plt.scatter(x_points_adv_intervals, y_points_10_bytes, color='g', marker="+", linewidth=5, label='10 bajtów danych')

    # 16 BYTES PLOT
    y_points_16_bytes = np.array(packet_loss_16bytes)
    plt.scatter(x_points_adv_intervals, y_points_16_bytes, color='b', marker="+", linewidth=5, label='16 bajtów danych')

    # Naming the x-axis, y-axis and the whole graph
    plt.xticks(np.arange(0, 1000, step=50))  # Set label locations.
    plt.grid()
    plt.xlabel("Interwał rozgłaszania[ms]")
    plt.ylabel("Utrata pakietów [%]")
    plt.title("Utrata pakietów dla różnych rozmiarów danych i różnych interwałów rozgłaszania")

    # Adding legend, which helps us recognize the curve according to it's color
    plt.legend()

    # To load the display window
    plt.show()


def plot_packet_losses_multiple_senders(packet_loss_data):
    x_points_adv_intervals = np.array([20, 50, 100, 300, 500, 1000])

    y_sender_a = []
    y_sender_b = []

    for data in packet_loss_data:
        y_sender_a.append(data[0].get_packet_loss())
        y_sender_b.append(data[1].get_packet_loss())

    plt.scatter(x_points_adv_intervals, y_sender_a, color='b', marker="+", linewidth=5, label='Nadawca A')
    plt.scatter(x_points_adv_intervals, y_sender_b, color='r', marker="+", linewidth=5, label='Nadawca B')

    # Naming the x-axis, y-axis and the whole graph
    plt.xticks(np.arange(0, 1050, step=50))  # Set label locations.
    plt.grid()
    plt.xlabel("Interwał rozgłaszania[ms]")
    plt.ylabel("Utrata pakietów [%]")
    plt.title("Utrata pakietów przy dwóch nadawcach")

    # Adding legend, which helps us recognize the curve according to it's color
    plt.legend()

    # To load the display window
    plt.show()


def plot_dq_fill_strategies(dq_data):
    x_points_adv_intervals = np.array([20, 50, 100, 300, 500, 1000])

    y_sender_a = []
    y_sender_b = []

    for data in dq_data:
        y_sender_a.append(data.random_key_fragment_selection)
        y_sender_b.append(data.sequence_key_fragment_selection)

    plt.scatter(x_points_adv_intervals, y_sender_a, color='b', marker="+", linewidth=5,
                label='Fragmenty klucza wybierane losowo')
    plt.scatter(x_points_adv_intervals, y_sender_b, color='r', marker="+", linewidth=5,
                label='Fragmenty klucza wybierane szeregowo')

    # Naming the x-axis, y-axis and the whole graph
    plt.xticks(np.arange(0, 1050, step=50))  # Set label locations.
    plt.grid()
    plt.xlabel("Interwał rozgłaszania danych [ms]")
    plt.ylabel("Zajętość kolejki odroczonych pakietów [%]")
    plt.title(
        "Badanie zmiany procentowej zajętości kolejki odroczonych pakietów dla różnych strategii wyboru fragmentów klucza")

    # Adding legend, which helps us recognize the curve according to it's color
    plt.legend()

    # To load the display window
    plt.show()


def plot_packet_losses_dynamics(packet_loss_range_dynamic):
    x_points_adv_intervals = ["20-200ms", "200-1000ms", "1000-3000ms", "3000-5000ms"]

    start_values = [20, 200, 1000, 3000]

    y_points_packet_losses = []

    i_start = 0
    while i_start != len(start_values):
        for data in packet_loss_range_dynamic:
            if data.sender_advertising_interval_min == start_values[i_start]:
                y_points_packet_losses.append(data.get_packet_loss())
                i_start += 1

    plt.bar(x_points_adv_intervals, y_points_packet_losses)
    plt.title('Utrata pakietów dla dynamicznego czasu rozgłaszania z danego zakresu')
    plt.xlabel('Zakres interwału rozgłaszania [ms]')
    plt.ylabel('Utrata pakietów [%]')
    plt.legend()
    plt.grid()
    plt.show()


def plot_packet_losses_dynamics_multiple_senders(packet_loss_range_dynamic):
    x_points_adv_intervals = ["20-200ms", "200-1000ms", "1000-3000ms", "3000-5000ms"]

    start_values = [20, 200, 1000, 3000]

    y_points_packet_losses_a = []
    y_points_packet_losses_b = []

    i_start = 0
    while i_start != len(start_values):
        for data in packet_loss_range_dynamic:
            if data[0].sender_advertising_interval_min == start_values[i_start]:
                y_points_packet_losses_a.append(data[0].get_packet_loss())
                y_points_packet_losses_b.append(data[1].get_packet_loss())
                i_start += 1

    X_axis = np.arange(len(x_points_adv_intervals))

    plt.bar(X_axis - 0.2, y_points_packet_losses_a, 0.4, label="Nadawca A")
    plt.bar(X_axis + 0.2, y_points_packet_losses_b, 0.4, label="Nadawca B")

    plt.title('Utrata pakietów dla dynamicznego czasu rozgłaszania z danego zakresu')
    plt.xlabel('Zakres interwału rozgłaszania [ms]')
    plt.ylabel('Utrata pakietów [%]')

    plt.xticks(X_axis + 0.2 / 2, ["20-200ms", "200-1000ms", "1000-3000ms", "3000-5000ms"])
    plt.legend()
    plt.grid()
    plt.show()

def plot_improved_algorithm_packet_losses_dynamics(packet_loss_range_dynamic):
    x_points_adv_intervals = ["20-200ms", "200-1000ms", "1000-3000ms", "3000-5000ms"]

    start_values = [20, 200, 1000, 3000]

    y_points_packet_losses = []

    i_start = 0
    while i_start != len(start_values):
        for data in packet_loss_range_dynamic:
            if data.sender_advertising_interval_min == start_values[i_start]:
                y_points_packet_losses.append(data.get_packet_loss())
                i_start += 1

    plt.bar(x_points_adv_intervals, y_points_packet_losses)
    plt.title('Utrata pakietów dla dynamicznego czasu rozgłaszania z danego zakresu')
    plt.xlabel('Zakres interwału rozgłaszania [ms]')
    plt.ylabel('Utrata pakietów [%]')
    plt.legend()
    plt.grid()
    plt.show()

def plot_improved_algorithm_packet_losses_dynamics_multiple_senders(packet_loss_range_dynamic):
    x_points_adv_intervals = ["20-200ms", "200-1000ms", "1000-3000ms", "3000-5000ms"]

    start_values = [20, 200, 1000, 3000]

    y_points_packet_losses_a = []
    y_points_packet_losses_b = []

    i_start = 0
    while i_start != len(start_values):
        for data in packet_loss_range_dynamic:
            if data[0].sender_advertising_interval_min == start_values[i_start]:
                y_points_packet_losses_a.append(data[0].get_packet_loss())
                y_points_packet_losses_b.append(data[1].get_packet_loss())
                i_start += 1

    X_axis = np.arange(len(x_points_adv_intervals))

    plt.bar(X_axis - 0.2, y_points_packet_losses_a, 0.4, label="Nadawca A")
    plt.bar(X_axis + 0.2, y_points_packet_losses_b, 0.4, label="Nadawca B")

    plt.title('Utrata pakietów dla dynamicznego czasu rozgłaszania z danego zakresu')
    plt.xlabel('Zakres interwału rozgłaszania [ms]')
    plt.ylabel('Utrata pakietów [%]')

    plt.xticks(X_axis + 0.2 / 2, ["20-200ms", "200-1000ms", "1000-3000ms", "3000-5000ms"])
    plt.legend()
    plt.grid()
    plt.show()


def print_packet_loss_tables_dynamic_multiple(packet_loss_dynamics_data):
    table_20_200ms = PrettyTable()
    table_20_200ms.field_names = ["Adv. Interval[ms]", "Packet Loss[%]"]

    table_200_1000ms = PrettyTable()
    table_200_1000ms.field_names = ["Adv. Interval[ms]", "Packet Loss[%]"]

    table_1000_3000ms = PrettyTable()
    table_1000_3000ms.field_names = ["Adv. Interval[ms]", "Packet Loss[%]"]

    table_3000_5000ms = PrettyTable()
    table_3000_5000ms.field_names = ["Adv. Interval[ms]", "Packet Loss[%]"]

    results = [table_20_200ms, table_200_1000ms, table_1000_3000ms, table_3000_5000ms]

    for data in packet_loss_dynamics_data:
        if data[0].sender_advertising_interval_min == 20:
            table_20_200ms.add_row(
                ["20 - 200 [ms]", data[0].get_packet_loss()])
            table_20_200ms.add_row(
                ["20 - 200 [ms]", data[1].get_packet_loss()])
        elif data[0].sender_advertising_interval_min == 200:
            table_200_1000ms.add_row(
                ["200 - 1000 [ms]", data[0].get_packet_loss()])
            table_200_1000ms.add_row(
                ["200 - 1000 [ms]", data[1].get_packet_loss()])
        elif data[0].sender_advertising_interval_min == 1000:
            table_1000_3000ms.add_row(
                ["1000 - 3000 [ms]", data[0].get_packet_loss()])
            table_1000_3000ms.add_row(
                ["1000 - 3000 [ms]", data[1].get_packet_loss()])
        elif data[0].sender_advertising_interval_min == 3000:
            table_3000_5000ms.add_row(
                ["3000 - 5000 [ms]", data[0].get_packet_loss()])
            table_3000_5000ms.add_row(
                ["3000 - 5000 [ms]", data[1].get_packet_loss()])

    print(table_20_200ms)
    print(table_200_1000ms)
    print(table_1000_3000ms)
    print(table_3000_5000ms)


def plot_key_reconstruction_average_time(adv_time_20ms, adv_time_50ms, adv_time_100ms, adv_time_300ms, adv_time_500ms,
                                         adv_time_1000ms):
    x_points_payload_sizes = np.array(TESTED_PAYLOAD_SIZES_IN_BYTES)

    # ADV INTERVALS PLOT
    y_points_20ms_bytes = np.array(adv_time_20ms)
    y_points_50ms_bytes = np.array(adv_time_50ms)
    y_points_100ms_bytes = np.array(adv_time_100ms)
    y_points_300ms_bytes = np.array(adv_time_300ms)
    y_points_500ms_bytes = np.array(adv_time_500ms)
    y_points_1000ms_bytes = np.array(adv_time_1000ms)

    fig, axs = plt.subplots(6, sharex=True, sharey=False)

    axs[0].scatter(x_points_payload_sizes, y_points_20ms_bytes, color='r', marker="+", linewidth=5)
    axs[0].grid()
    axs[0].set_title('Adv. interval 20ms')

    axs[1].scatter(x_points_payload_sizes, y_points_50ms_bytes, color='r', marker="+", linewidth=5)
    axs[1].grid()
    axs[1].set_title('Adv. interval 50ms')

    axs[2].scatter(x_points_payload_sizes, y_points_100ms_bytes, color='g', marker="+", linewidth=5)
    axs[2].grid()
    axs[2].set_title('Adv. interval 100ms')

    axs[3].scatter(x_points_payload_sizes, y_points_300ms_bytes, color='b', marker="+", linewidth=5)
    axs[3].grid()
    axs[3].set_title('Adv. interval 300ms')

    axs[4].scatter(x_points_payload_sizes, y_points_500ms_bytes, color='black', marker="+", linewidth=5)
    axs[4].grid()
    axs[4].set_title('Adv. interval 500ms')

    axs[5].scatter(x_points_payload_sizes, y_points_1000ms_bytes, color='purple', marker="+", linewidth=5)
    axs[5].grid()
    axs[5].set_title('Adv. interval 1000ms')

    fig.supxlabel('Paylod Size [B]')
    fig.supylabel('Key Reconstruction Average Time [s]')
    fig.suptitle('Key Reconstruction Average Time for different payload sizes')
    fig.legend()
    plt.show()


def plot_dq_fill_percentage(adv_time_20ms, adv_time_50ms, adv_time_100ms, adv_time_300ms, adv_time_500ms,
                            adv_time_1000ms):
    x_points_payload_sizes = np.array(TESTED_PAYLOAD_SIZES_IN_BYTES)

    # ADV INTERVALS PLOT
    y_points_20ms_bytes = np.array(adv_time_20ms)
    y_points_50ms_bytes = np.array(adv_time_50ms)
    y_points_100ms_bytes = np.array(adv_time_100ms)
    y_points_300ms_bytes = np.array(adv_time_300ms)
    y_points_500ms_bytes = np.array(adv_time_500ms)
    y_points_1000ms_bytes = np.array(adv_time_1000ms)

    fig, axs = plt.subplots(6, sharex=True, sharey=False)

    axs[0].scatter(x_points_payload_sizes, y_points_20ms_bytes, color='r', marker="+", linewidth=5)
    axs[0].grid()
    axs[0].set_title('Adv. interval 20ms')

    axs[1].scatter(x_points_payload_sizes, y_points_50ms_bytes, color='r', marker="+", linewidth=5)
    axs[1].grid()
    axs[1].set_title('Adv. interval 50ms')

    axs[2].scatter(x_points_payload_sizes, y_points_100ms_bytes, color='g', marker="+", linewidth=5)
    axs[2].grid()
    axs[2].set_title('Adv. interval 100ms')

    axs[3].scatter(x_points_payload_sizes, y_points_300ms_bytes, color='b', marker="+", linewidth=5)
    axs[3].grid()
    axs[3].set_title('Adv. interval 300ms')

    axs[4].scatter(x_points_payload_sizes, y_points_500ms_bytes, color='black', marker="+", linewidth=5)
    axs[4].grid()
    axs[4].set_title('Adv. interval 500ms')

    axs[5].scatter(x_points_payload_sizes, y_points_1000ms_bytes, color='purple', marker="+", linewidth=5)
    axs[5].grid()
    axs[5].set_title('Adv. interval 1000ms')

    fig.supxlabel('Paylod Size [B]')
    fig.supylabel('Deferred Queue Average Fill [%]')
    fig.suptitle('Deferred Queue Average Fill to Key Reconstruction Complete')
    fig.legend()
    plt.show()


def plot_dq_fill_percentage_one_plot(data):
    x_points_adv_intervals = np.array(TESTED_ADVERTISING_INTERVALS_IN_MS)

    y_points = np.array(data)
    plt.scatter(x_points_adv_intervals, y_points, color='r', marker="+", linewidth=5)

    # Naming the x-axis, y-axis and the whole graph
    plt.xticks(np.arange(0, 1000, step=50))  # Set label locations.
    plt.grid()
    plt.xlabel("Adv time interval[ms]")
    plt.ylabel("Deferred Queue Fill[%]")
    plt.title("Deferred Queue Fill at different payload sizes and adv intervals")

    # Adding legend, which helps us recognize the curve according to it's color
    plt.legend()

    # To load the display window
    plt.show()


def print_packet_loss_tables(packet_loss_4bytes, packet_loss_10bytes, packet_loss_16bytes):
    table_20ms = PrettyTable()
    table_20ms.field_names = ["Adv. Interval[ms]", "Payload [B]", "Packet Loss[%]"]

    table_50ms = PrettyTable()
    table_50ms.field_names = ["Adv. Interval[ms]", "Payload [B]", "Packet Loss[%]"]

    table_100ms = PrettyTable()
    table_100ms.field_names = ["Adv. Interval[ms]", "Payload [B]", "Packet Loss[%]"]

    table_300ms = PrettyTable()
    table_300ms.field_names = ["Adv. Interval[ms]", "Payload [B]", "Packet Loss[%]"]

    table_500ms = PrettyTable()
    table_500ms.field_names = ["Adv. Interval[ms]", "Payload [B]", "Packet Loss[%]"]

    table_1000ms = PrettyTable()
    table_1000ms.field_names = ["Adv. Interval[ms]", "Payload [B]", "Packet Loss[%]"]

    results = [packet_loss_4bytes, packet_loss_10bytes, packet_loss_16bytes]

    for result in results:
        for data in result:
            if data.sender_advertising_interval_ms == 20:
                table_20ms.add_row(
                    [data.sender_advertising_interval_ms, data.sender_payload_size, data.get_packet_loss()])
            elif data.sender_advertising_interval_ms == 50:
                table_50ms.add_row(
                    [data.sender_advertising_interval_ms, data.sender_payload_size, data.get_packet_loss()])
            elif data.sender_advertising_interval_ms == 100:
                table_100ms.add_row(
                    [data.sender_advertising_interval_ms, data.sender_payload_size, data.get_packet_loss()])
            elif data.sender_advertising_interval_ms == 300:
                table_300ms.add_row(
                    [data.sender_advertising_interval_ms, data.sender_payload_size, data.get_packet_loss()])
            elif data.sender_advertising_interval_ms == 500:
                table_500ms.add_row(
                    [data.sender_advertising_interval_ms, data.sender_payload_size, data.get_packet_loss()])
            elif data.sender_advertising_interval_ms == 1000:
                table_1000ms.add_row(
                    [data.sender_advertising_interval_ms, data.sender_payload_size, data.get_packet_loss()])

    print(table_20ms)
    print(table_50ms)
    print(table_100ms)
    print(table_300ms)
    print(table_500ms)
    print(table_1000ms)


def print_packet_loss_tables_dynamic(packet_loss_dynamics_data):
    table_20_200ms = PrettyTable()
    table_20_200ms.field_names = ["Adv. Interval[ms]", "Packet Loss[%]"]

    table_200_1000ms = PrettyTable()
    table_200_1000ms.field_names = ["Adv. Interval[ms]", "Packet Loss[%]"]

    table_1000_3000ms = PrettyTable()
    table_1000_3000ms.field_names = ["Adv. Interval[ms]", "Packet Loss[%]"]

    table_3000_5000ms = PrettyTable()
    table_3000_5000ms.field_names = ["Adv. Interval[ms]", "Packet Loss[%]"]

    results = [table_20_200ms, table_200_1000ms, table_1000_3000ms, table_3000_5000ms]

    for data in packet_loss_dynamics_data:
        if data.sender_advertising_interval_min == 20:
            table_20_200ms.add_row(
                ["20 - 200 [ms]", data.get_packet_loss()])
        elif data.sender_advertising_interval_min == 200:
            table_200_1000ms.add_row(
                ["200 - 1000 [ms]", data.get_packet_loss()])
        elif data.sender_advertising_interval_min == 1000:
            table_1000_3000ms.add_row(
                ["1000 - 3000 [ms]", data.get_packet_loss()])
        elif data.sender_advertising_interval_min == 3000:
            table_3000_5000ms.add_row(
                ["3000 - 5000 [ms]", data.get_packet_loss()])

    print(table_20_200ms)
    print(table_200_1000ms)
    print(table_1000_3000ms)
    print(table_3000_5000ms)


def print_packet_loss_tables_multiple_senders(packet_loss_data):
    table_20ms = PrettyTable()
    table_20ms.field_names = ["Adv. Interval [ms]", "Packet Loss[%]"]

    table_50ms = PrettyTable()
    table_50ms.field_names = ["Adv. Interval [ms]", "Packet Loss[%]"]

    table_100ms = PrettyTable()
    table_100ms.field_names = ["Adv. Interval [ms]", "Packet Loss[%]"]

    table_300ms = PrettyTable()
    table_300ms.field_names = ["Adv. Interval [ms]", "Packet Loss[%]"]

    table_500ms = PrettyTable()
    table_500ms.field_names = ["Adv. Interval [ms]", "Packet Loss[%]"]

    table_1000ms = PrettyTable()
    table_1000ms.field_names = ["Adv. Interval [ms]", "Packet Loss[%]"]

    for data in packet_loss_data:
        if data[0].sender_advertising_interval_ms == 20:
            table_20ms.add_row(
                ["Sender A 20 [ms]", data[0].get_packet_loss()])
            table_20ms.add_row(
                ["Sender B 20 [ms]", data[1].get_packet_loss()])
        elif data[0].sender_advertising_interval_ms == 50:
            table_50ms.add_row(
                ["Sender A 50 [ms]", data[0].get_packet_loss()])
            table_50ms.add_row(
                ["Sender B 50 [ms]", data[1].get_packet_loss()])
        elif data[0].sender_advertising_interval_ms == 100:
            table_100ms.add_row(
                ["Sender A 100 [ms]", data[0].get_packet_loss()])
            table_100ms.add_row(
                ["Sender B 100 [ms]", data[1].get_packet_loss()])
        elif data[0].sender_advertising_interval_ms == 300:
            table_300ms.add_row(
                ["Sender A 300 [ms]", data[0].get_packet_loss()])
            table_300ms.add_row(
                ["Sender B 300 [ms]", data[1].get_packet_loss()])
        elif data[0].sender_advertising_interval_ms == 500:
            table_500ms.add_row(
                ["Sender A 500 [ms]", data[0].get_packet_loss()])
            table_500ms.add_row(
                ["Sender B 500 [ms]", data[1].get_packet_loss()])
        elif data[0].sender_advertising_interval_ms == 1000:
            table_1000ms.add_row(
                ["Sender A 1000 [ms]", data[0].get_packet_loss()])
            table_1000ms.add_row(
                ["Sender B 1000 [ms]", data[1].get_packet_loss()])

    print(table_20ms)
    print(table_50ms)
    print(table_100ms)
    print(table_300ms)
    print(table_500ms)
    print(table_1000ms)


if __name__ == "__main__":
    # plot_packet_losses_dynamics_multiple_senders(results.MULTIPLE_SENDERS_DYNAMICS)
    # print_packet_loss_tables_dynamic_multiple(results.MULTIPLE_SENDERS_DYNAMICS)
    # #GET PACKET LOSSES
    # PACKET_LOSS_4_BYTES, PACKET_LOSS_10_BYTES, PACKET_LOSS_16_BYTES = get_packet_losses()
    #
    # plot_packet_losses(PACKET_LOSS_4_BYTES, PACKET_LOSS_10_BYTES, PACKET_LOSS_16_BYTES)
    #
    # # GET AVERAGE RECONSTRUCTION TIME
    # AVERAGE_RECONSTRUCTION_TIME_FOR_20MS = get_average_reconstruction_time_for_interval(20)
    # AVERAGE_RECONSTRUCTION_TIME_FOR_50MS = get_average_reconstruction_time_for_interval(50)
    # AVERAGE_RECONSTRUCTION_TIME_FOR_100MS = get_average_reconstruction_time_for_interval(100)
    # AVERAGE_RECONSTRUCTION_TIME_FOR_300MS = get_average_reconstruction_time_for_interval(300)
    # AVERAGE_RECONSTRUCTION_TIME_FOR_500MS = get_average_reconstruction_time_for_interval(500)
    # AVERAGE_RECONSTRUCTION_TIME_FOR_1000MS = get_average_reconstruction_time_for_interval(1000)
    #
    # plot_key_reconstruction_average_time(
    #     AVERAGE_RECONSTRUCTION_TIME_FOR_20MS,
    #     AVERAGE_RECONSTRUCTION_TIME_FOR_50MS,
    #     AVERAGE_RECONSTRUCTION_TIME_FOR_100MS,
    #     AVERAGE_RECONSTRUCTION_TIME_FOR_300MS,
    #     AVERAGE_RECONSTRUCTION_TIME_FOR_500MS,
    #     AVERAGE_RECONSTRUCTION_TIME_FOR_1000MS
    # )
    #
    # # GET AVERAGE DQ FILL
    # AVERAGE_DQ_FILL_FOR_20MS = get_average_dq_fill_for_interval(20)
    # AVERAGE_DQ_FILL_FOR_50MS = get_average_dq_fill_for_interval(50)
    # AVERAGE_DQ_FILL_FOR_100MS = get_average_dq_fill_for_interval(100)
    # AVERAGE_DQ_FILL_FOR_300MS = get_average_dq_fill_for_interval(300)
    # AVERAGE_DQ_FILL_FOR_500MS = get_average_dq_fill_for_interval(500)
    # AVERAGE_DQ_FILL_FOR_1000MS = get_average_dq_fill_for_interval(1000)
    #
    # plot_dq_fill_percentage(
    #     AVERAGE_DQ_FILL_FOR_20MS,
    #     AVERAGE_DQ_FILL_FOR_50MS,
    #     AVERAGE_DQ_FILL_FOR_100MS,
    #     AVERAGE_DQ_FILL_FOR_300MS,
    #     AVERAGE_DQ_FILL_FOR_500MS,
    #     AVERAGE_DQ_FILL_FOR_1000MS
    # )
    #
    # dq_avg = [
    #     statistics.mean(AVERAGE_DQ_FILL_FOR_20MS),
    #     statistics.mean(AVERAGE_DQ_FILL_FOR_50MS),
    #     statistics.mean(AVERAGE_DQ_FILL_FOR_100MS),
    #     statistics.mean(AVERAGE_DQ_FILL_FOR_300MS),
    #     statistics.mean(AVERAGE_DQ_FILL_FOR_500MS),
    #     statistics.mean(AVERAGE_DQ_FILL_FOR_1000MS)
    # ]
    #
    # plot_dq_fill_percentage_one_plot(dq_avg)
    #
    # plot_dq_fill_strategies(results.AVERAGE_QUEUE_FILL_DATA)
    #
    # print("============PACKET LOSS STATIC============")
    # print_packet_loss_tables(results.TEST_DATA_4_BYTES, results.TEST_DATA_10_BYTES, results.TEST_DATA_16_BYTES)
    #
    # plot_packet_losses_dynamics(results.DYNAMIC_TESTS_RESULTS)
    #
    # plot_packet_losses_multiple_senders(results.MULTIPLE_SENDERS_RESULTS)
    #
    # print("============PACKET LOSS DYNAMIC============")
    # print_packet_loss_tables_dynamic(results.DYNAMIC_TESTS_RESULTS)
    # print("============PACKET LOSS MULTIPLE RECEIVERS============")
    # print_packet_loss_tables_multiple_senders(results.MULTIPLE_SENDERS_RESULTS)

    plot_improved_algorithm_packet_losses_dynamics(results.IMPROVED_ALGORITHM_DYNAMIC_TESTS_RESULTS)

    plot_improved_algorithm_packet_losses_dynamics_multiple_senders(results.IMPROVED_ALGORITHM_MULTIPLE_SENDERS_DYNAMICS)