import socket
import time
from PyQt5.QtCore import QThread, pyqtSignal
from controller_state import ControllerState
from formatter import UDPFormatter
from logger import Logger
from settings import ESP32_HOST, ESP32_PORT, TICK_HZ


class UDPSender(QThread):
    """UDP sender for transmitting controller data to ESP32."""
    
    # Signals
    connection_status_changed = pyqtSignal(str)  # Emits "connected" or "disconnected"
    
    def __init__(self, controller_state: ControllerState, logger: Logger):
        super().__init__()
        self.controller_state = controller_state
        self.logger = logger
        self.formatter = UDPFormatter(logger)
        
        self.running = False
        self.socket = None
        self.target_address = (ESP32_HOST, ESP32_PORT)
        self.send_interval = 1.0 / TICK_HZ  # Send rate based on TICK_HZ
        self._err_log_interval = 1.0 # Prevent flooding log error
        self._next_err_log = 0.0
        
        self.logger.info(f"UDP Sender initialized (target: {ESP32_HOST}:{ESP32_PORT}, rate: {TICK_HZ}Hz)")
    
    def start_sending(self):
        """Start UDP transmission."""
        if not self.running:
            self.running = True
            self.start()  # Start QThread
            self.logger.info("UDP sender started")
    
    def stop_sending(self):
        """Stop UDP transmission."""
        self.running = False
        if self.isRunning():
            self.wait(2000)  # Wait up to 2 seconds
        self.logger.info("UDP sender stopped")
    
    def run(self):
        """Main UDP sender loop"""
        self.logger.info("UDP sender thread started")
        if not self._create_socket():
            return
        packet_count = 0
        while self.running:
            try:
                packet = self.formatter.format_packet(self.controller_state)
                if len(packet) != 28:
                    self.logger.failure(f"UDP packet size unexpected: {len(packet)} bytes (expected 28)")
                self._send_packet(packet, packet_count)
                packet_count += 1
                time.sleep(self.send_interval)
            except Exception as e:
                self.logger.failure(f"Error in UDP sender: {e}")
                time.sleep(0.1)
        self._close_socket()
        self.logger.info("UDP sender thread finished")
    
    def _create_socket(self) -> bool:
        """Create UDP socket."""
        try:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self.socket.settimeout(1.0)  # 1 second timeout
            self.logger.success(f"UDP socket created for {self.target_address}")
            self.connection_status_changed.emit("connected")
            return True
        except Exception as e:
            self.logger.failure(f"Failed to create UDP socket: {e}")
            self.connection_status_changed.emit("disconnected")
            return False
    
    def _send_packet(self, packet: bytes, packet_count: int):
        """Send UDP packets and logs (first 5 packets then 1 every second)"""
        try:
            if packet_count % 60 == 0 or packet_count < 5:
                self._log_packet_details(packet, packet_count + 1)

            self.socket.sendto(packet, self.target_address)

            if packet_count % 300 == 0:
                self.connection_status_changed.emit("connected")
        except socket.timeout:
            self.logger.failure(f"UDP send timeout to {self.target_address}")
            self.connection_status_changed.emit("disconnected")
        except socket.error as e:
            now = time.monotonic()
            if now >= self._next_err_log:
                try:
                    self.logger.failure(f"UDP send error: {e}")
                except Exception:
                    pass  # prevents crash when logger destroyed
                self._next_err_log = now + self.err_log_interval
            self.connection_status_changed.emit("disconnected")
        except Exception as e:
            self.logger.failure(f"Unexpected error sending UDP: {e}")
    
    def _log_packet_details(self, packet: bytes, packet_count: int):
        """Log detailed packet information."""
        # Convert bytes to hex string
        hex_bytes = ' '.join(f'{b:02x}' for b in packet)
        
        # Log the raw bytes
        self.logger.info(f"UDP packet #{packet_count} (28 bytes): {hex_bytes}")
        
        # Log decoded packet for readability
        decoded = self.formatter.debug_packet(packet)
        self.logger.info(f"Decoded: {decoded}")
    
    def _close_socket(self):
        """Close UDP socket."""
        if self.socket:
            try:
                self.socket.close()
                self.socket = None
                self.logger.info("UDP socket closed")
                self.connection_status_changed.emit("disconnected")
            except Exception as e:
                self.logger.failure(f"Error closing UDP socket: {e}")
    
    def get_connection_info(self) -> dict:
        """Get UDP connection information."""
        return {
            "target_host": ESP32_HOST,
            "target_port": ESP32_PORT,
            "socket_active": self.socket is not None,
            "send_rate_hz": TICK_HZ
        }