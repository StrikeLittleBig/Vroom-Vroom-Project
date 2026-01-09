import struct
from controller_state import ControllerState
from logger import Logger
from settings import UDP_PACKET_FORMAT, UDP_BUTTON_BITS
BIT_TO_NAME = {bit: name for name, bit in UDP_BUTTON_BITS.items()}


class UDPFormatter:
    """Formats controller state into UDP packets."""
    
    def __init__(self, logger: Logger):
        self.logger = logger
        self._struct_format = UDP_PACKET_FORMAT["struct"]
        self._packet_size = UDP_PACKET_FORMAT["size"]
        
        # Validate struct format
        expected_size = struct.calcsize(self._struct_format)
        if expected_size != self._packet_size:
            self.logger.failure(f"Struct size mismatch: expected {self._packet_size}, got {expected_size}")
        else:
            self.logger.success(f"UDP formatter initialized (packet size: {self._packet_size} bytes)")
    
    def format_packet(self, controller_state: ControllerState) -> bytes:
        """
        Format controller state into UDP packet.
        
        Args:
            controller_state: Current controller state
            
        Returns:
            28-byte UDP packet as bytes
        """
        try:
            # Get UDP state data
            state_data = controller_state.get_udp_state()
            
            # Pack into binary format: <I6f (uint32 + 6 float32)
            packet = struct.pack(
                self._struct_format,
                state_data["buttons"],          # uint32
                state_data["left_stick_x"],     # float32
                state_data["left_stick_y"],     # float32  
                state_data["right_stick_x"],    # float32
                state_data["right_stick_y"],    # float32
                state_data["left_trigger"],     # float32
                state_data["right_trigger"]     # float32
            )
            
            return packet
            
        except Exception as e:
            self.logger.failure(f"Failed to format UDP packet: {e}")
            return self._create_empty_packet()
    
    def _create_empty_packet(self) -> bytes:
        """Create an empty/neutral UDP packet."""
        return struct.pack(self._struct_format, 0, 0.0, 0.0, 0.0, 0.0, -1.0, -1.0)
        
    def debug_packet(self, packet: bytes) -> str:
        """
        Debug function to decode and display packet contents.
        
        Args:
            packet: UDP packet bytes
            
        Returns:
            Human-readable packet description
        """
        try:
            if len(packet) != self._packet_size:
                return f"Invalid packet size: {len(packet)} (expected {self._packet_size})"
            buttons, lx, ly, rx, ry, lt, rt = struct.unpack(self._struct_format, packet)
            pressed = [BIT_TO_NAME[b] for b in range(32) if (buttons >> b) & 1 and b in BIT_TO_NAME]
            return (f"Buttons(mask=0x{buttons:08X}): {pressed}, "
                    f"L=({lx:.2f},{ly:.2f}), R=({rx:.2f},{ry:.2f}), Trig=({lt:.2f},{rt:.2f})")
        except Exception as e:
            return f"Failed to debug packet: {e}"