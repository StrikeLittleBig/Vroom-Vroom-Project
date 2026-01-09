from dataclasses import dataclass, field
from typing import Dict, Any, Optional
from logger import Logger


@dataclass
class ControllerState:
    """Minimal controller state for gamepad input management."""
    
    # Analog inputs (normalized -1.0 to 1.0)
    left_stick_x: float = 0.0
    left_stick_y: float = 0.0
    right_stick_x: float = 0.0
    right_stick_y: float = 0.0
    left_trigger: float = -1.0   # -1 = released, 1 = fully pressed
    right_trigger: float = -1.0
    
    # Digital inputs
    buttons: Dict[str, bool] = field(default_factory=dict)
    dpad_direction: int = -1  # 0=Up, 1=Right, 2=Down, 3=Left, -1=Neutral
    
    # Connection state
    connected: bool = False
    
    # Logger (optional)
    _logger: Optional[Logger] = field(default=None, init=False)
    
    def __post_init__(self):
        """Initialize button states."""
        expected_buttons = ["A", "B", "X", "Y", "LB", "RB", "Share", "Select", "Xbox", "LS", "RS"]
        for button in expected_buttons:
            if button not in self.buttons:
                self.buttons[button] = False
    
    def set_logger(self, logger: Logger):
        """Set logger for state changes."""
        self._logger = logger
    
    def update_connection(self, connected: bool):
        """Update connection status."""
        if self.connected != connected:
            self.connected = connected
            if self._logger:
                if connected:
                    self._logger.success("Controller connected")
                else:
                    self._logger.failure("Controller disconnected")
                    self._reset_inputs()
    
    def update_sticks(self, left_x=None, left_y=None, right_x=None, right_y=None):
        """Update stick positions."""
        if left_x is not None:
            self.left_stick_x = max(-1.0, min(1.0, left_x))
        if left_y is not None:
            self.left_stick_y = max(-1.0, min(1.0, left_y))
        if right_x is not None:
            self.right_stick_x = max(-1.0, min(1.0, right_x))
        if right_y is not None:
            self.right_stick_y = max(-1.0, min(1.0, right_y))
    
    def update_triggers(self, left_trigger=None, right_trigger=None):
        """Update trigger values."""
        if left_trigger is not None:
            self.left_trigger = max(-1.0, min(1.0, left_trigger))
        if right_trigger is not None:
            self.right_trigger = max(-1.0, min(1.0, right_trigger))
    
    def update_button(self, button_name: str, pressed: bool):
        """Update button state."""
        self.buttons[button_name] = pressed
    
    def update_dpad(self, direction: int):
        """Update D-pad direction."""
        self.dpad_direction = direction
    
    def get_ui_state(self) -> Dict[str, Any]:
        """Get state for UI display."""
        # Convert buttons to bitmask for UI
        buttons_bitmask = 0
        ui_mapping = {"A": 0, "B": 1, "X": 2, "Y": 3, "LB": 4, "RB": 5, 
                     "Share": 6, "Select": 7, "LS": 8, "RS": 9, "Xbox": 10}
        
        for button, bit in ui_mapping.items():
            if self.buttons.get(button, False):
                buttons_bitmask |= (1 << bit)
        
        return {
            "lx": self.left_stick_x,
            "ly": self.left_stick_y,
            "rx": self.right_stick_x,
            "ry": self.right_stick_y,
            "lt": self.left_trigger,
            "rt": self.right_trigger,
            "buttons": buttons_bitmask,
            "dpad": self.dpad_direction,
            "connected": self.connected
        }
    
    def get_udp_state(self) -> Dict[str, Any]:
        """Get state for UDP transmission."""
        # Convert buttons to UDP bitmask
        buttons_bitmask = 0
        udp_mapping = {"X": 0, "Y": 1, "A": 2, "B": 3, "LB": 4, "RB": 5,
                      "Share": 6, "Select": 7, "Xbox": 8, "LS": 13, "RS": 14}
        
        # Regular buttons
        for button, bit in udp_mapping.items():
            if self.buttons.get(button, False):
                buttons_bitmask |= (1 << bit)
        
        # D-pad buttons
        if self.dpad_direction == 0:    # Up
            buttons_bitmask |= (1 << 9)
        elif self.dpad_direction == 1:  # Right
            buttons_bitmask |= (1 << 11)
        elif self.dpad_direction == 2:  # Down
            buttons_bitmask |= (1 << 12)
        elif self.dpad_direction == 3:  # Left
            buttons_bitmask |= (1 << 10)
        
        return {
            "buttons": buttons_bitmask,
            "left_stick_x": self.left_stick_x,
            "left_stick_y": self.left_stick_y,
            "right_stick_x": self.right_stick_x,
            "right_stick_y": self.right_stick_y,
            "left_trigger": self.left_trigger,
            "right_trigger": self.right_trigger
        }
    
    def _reset_inputs(self):
        """Reset all inputs to neutral."""
        self.left_stick_x = self.left_stick_y = 0.0
        self.right_stick_x = self.right_stick_y = 0.0
        self.left_trigger = self.right_trigger = -1.0
        self.dpad_direction = -1
        for button in self.buttons:
            self.buttons[button] = False