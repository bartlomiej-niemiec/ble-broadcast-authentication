import serial
import serial.threaded
import time
import csv
import queue

SERIAL_PORT_CONFIG = {
    'port': 'COM3',
    'baudrate': 115200,
    'bytesize': serial.EIGHTBITS,
    'stopbits': serial.STOPBITS_ONE,
    'parity': serial.PARITY_NONE
}

FILENAME = "test_sender"
TIMESTR = time.strftime("%Y%m%d_%H%M%S")
LOGFILEPATH = FILENAME + "_" + TIMESTR + ".csv"

MAX_Q_SIZE = 20

START_CMD = "START_TEST"
CSV_FIELDS = ["timestamp", "message", "data"]
CSV_DICT_TEMPLATE = {
    "timestamp": "",
    "message": "",
    "data": ""
}


class LogWriter:

    def __init__(self, queue):
        self._start_logging_msg = "main_task: Calling app_main"
        self._start_logging_msg_checked = False
        self._queue = queue

    def _is_log_start(self, data):
        if not self._start_logging_msg_checked:
            if self._start_logging_msg in data:
                self._start_logging_msg_checked = True
        return self._start_logging_msg_checked

    def add_data_to_logging_queue(self, data):
        row_to_write = None
        if self._is_log_start(data):
            self._queue.put(self._extract_data_from_serial(data))

    def _extract_data_from_serial(self, data):
        row_to_write = CSV_DICT_TEMPLATE.copy()
        i = 0
        while data[i] != '(':
            i += 1
        i += 1
        timestamp = ""
        while data[i] != ')':
            timestamp += data[i]
            i += 1

        row_to_write["timestamp"] = timestamp
        rest_data = data[i:]
        splitted_rest_data = rest_data.split(":")
        if len(splitted_rest_data) > 2:
            row_to_write["message"] = splitted_rest_data[1].strip()
            row_to_write["data"] = splitted_rest_data[2].strip()
        else:
            row_to_write["message"] = splitted_rest_data[1].strip()

        return row_to_write


class Esp32PrintLines(serial.threaded.LineReader):
    TERMINATOR = b'\r\n'
    ENCODING = 'utf-8'
    UNICODE_HANDLING = 'replace'

    def __init__(self, log_writer):
        super().__init__()
        self._log_writer = log_writer

    def connection_made(self, transport):
        super(Esp32PrintLines, self).connection_made(transport)
        print('port opened\n')

    def handle_line(self, data):
        data_str = "{}".format(repr(data))
        self._log_writer.add_data_to_logging_queue(data_str)

    def connection_lost(self, exc):
        print(f"Connection has been lost: {exc}")

    def write_line(self, text):
        """
        Write text to the transport. ``text`` is a Unicode string and the encoding
        is applied before sending and also the newline is append.
        """
        # + is not the best choice but bytes does not support % or .format in py3 and we want a single write call
        self.transport.write(text.encode(self.ENCODING, self.UNICODE_HANDLING))


class Esp32ProtocolFactory:

    QUEUE = None

    @classmethod
    def register_queue(cls, queue):
        cls.QUEUE = queue


    @classmethod
    def create_protocol(cls):
        return Esp32PrintLines(LogWriter(cls.QUEUE))


if __name__ == "__main__":

    serial_port = serial.Serial(
        SERIAL_PORT_CONFIG['port'],
        SERIAL_PORT_CONFIG['baudrate'],
        SERIAL_PORT_CONFIG['bytesize'],
        SERIAL_PORT_CONFIG['parity'],
        SERIAL_PORT_CONFIG['stopbits'],
    )

    queue = queue.Queue(maxsize=MAX_Q_SIZE)
    Esp32ProtocolFactory.register_queue(queue)
    t = serial.threaded.ReaderThread(serial_port, Esp32ProtocolFactory.create_protocol)
    with t as protocol:
        with open(LOGFILEPATH, 'w', newline='', encoding='utf-8') as csvfile:
            csv_writer = csv.DictWriter(csvfile, fieldnames=CSV_FIELDS, delimiter=';')
            csv_writer.writeheader()
            start_time = time.time()
            cmd_start_send = False
            while True:
                if not queue.empty():
                    data = queue.get()
                    print(data)
                    csv_writer.writerow(data)

                if cmd_start_send is False:
                    if time.time() - start_time > 10:
                        protocol.write_line(START_CMD)
                        cmd_start_send = True

                time.sleep(0.025)

