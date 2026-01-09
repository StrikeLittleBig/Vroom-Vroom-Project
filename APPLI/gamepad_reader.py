import pygame
import time
from PyQt5.QtCore import QThread, pyqtSignal
from controller_state import ControllerState
from logger import Logger
from settings import PYGAME_BUTTON_MAPPING, DPAD_MAPPING, POLL_HZ


class GamepadReader(QThread):
    """Pygame-based gamepad reader running in separate thread."""
    
    # Signal emitted when controller state changes
    state_changed = pyqtSignal(dict)  # Emits UI state dict
    connection_changed = pyqtSignal(bool)  # Emits True/False on connect/disconnect
    
    def __init__(self, controller_state: ControllerState, logger: Logger):
        super().__init__()
        self.controller_state = controller_state
        self.logger = logger
        self.running = False
        self._joystick = None
        self._poll_interval = 1.0 / POLL_HZ  # Convert Hz to seconds
        
        # Initialize pygame
        self._init_pygame()
    
    def _init_pygame(self):
        """Initialize pygame joystick module."""
        try:
            pygame.init()
            pygame.joystick.init()
            self.logger.success("Pygame initialized")
        except Exception as e:
            self.logger.failure(f"Failed to initialize pygame: {e}")
            raise
    
    def start_reading(self):
        """Start the gamepad reading thread."""
        if not self.running:
            self.running = True
            self.start()  # Start QThread
            self.logger.info("Gamepad reader started")
    
    def stop_reading(self):
        """Stop the gamepad reading thread."""
        self.running = False
        if self.isRunning():
            self.wait(2000)  # Wait up to 2 seconds for thread to finish
        self.logger.info("Gamepad reader stopped")
    
    def run(self):
        """Main thread loop - reads gamepad and updates state."""
        try:
            if not pygame.get_init():
                pygame.init()
            if not pygame.joystick.get_init():
                pygame.joystick.init()
            self.logger.success("Pygame initialized")
        except Exception as e:
            self.logger.failure(f"Pygame init failed: {e}")
            return

        self.running = True
        self.logger.info("Gamepad reader started")
        
        while self.running:
            try:
                pygame.event.pump() 
                # Check for controller connection
                self._check_connection()
                self.connection_changed.emit(self.controller_state.connected)
                
                # Read inputs if connected
                if self._joystick and self.controller_state.connected:
                    self._read_inputs()
                    
                    # Emit state change signal
                    ui_state = self.controller_state.get_ui_state()
                    self.state_changed.emit(ui_state)
                
                # Sleep to maintain polling rate
                time.sleep(self._poll_interval)
                
            except Exception as e:
                self.logger.failure(f"Error in gamepad reader: {e}")
                time.sleep(0.1)  # Brief pause before retry
        
        # Cleanup
        self._disconnect_controller()
        self.logger.info("Gamepad reader thread finished")
    
    def _check_connection(self):
        """Check for controller connection/disconnection."""   
        pygame.event.pump() 
        joystick_count = pygame.joystick.get_count()
        
        if joystick_count > 0 and not self.controller_state.connected:
            # Controller connected
            try:
                self._joystick = pygame.joystick.Joystick(0)
                self._joystick.init()
                self.controller_state.update_connection(True)
                self.logger.success(f"Controller connected: {self._joystick.get_name()}")
                self.connection_changed.emit(True)
            except Exception as e:
                self.logger.failure(f"Failed to initialize controller: {e}")
                self._joystick = None
        
        elif joystick_count == 0 and self.controller_state.connected:
            # Controller disconnected
            self._disconnect_controller()
    
    def _disconnect_controller(self):
        """Handle controller disconnection."""
        if self._joystick:
            try:
                self._joystick.quit()
            except:
                pass
            self._joystick = None
        
        self.controller_state.update_connection(False)
        self.connection_changed.emit(False)
    
    def _read_inputs(self):
        """Read all controller inputs and update state."""
        if not self._joystick or not self._joystick.get_init():
            return
        pygame.event.pump()
        
        try:
            # Process pygame events (required for input updates)
            pygame.event.pump()
            
            # Read buttons
            self._read_buttons()
            
            # Read analog sticks
            self._read_sticks()
            
            # Read triggers
            self._read_triggers()
            
            # Read D-pad
            self._read_dpad()
            
        except Exception as e:
            self.logger.failure(f"Error reading inputs: {e}")
    
    def _read_buttons(self):
        """Read button states."""
        for pygame_btn_id, button_name in PYGAME_BUTTON_MAPPING.items():
            if pygame_btn_id < self._joystick.get_numbuttons():
                pressed = self._joystick.get_button(pygame_btn_id) == 1
                self.controller_state.update_button(button_name, pressed)
    
    def _read_sticks(self):
        """Read analog stick positions."""
        # Get axis count to avoid index errors
        num_axes = self._joystick.get_numaxes()
        
        # Left stick
        if 0 < num_axes:  # Left stick X
            left_x = self._joystick.get_axis(0)
            left_y = -self._joystick.get_axis(1) if 1 < num_axes else 0.0  # Invert Y
            self.controller_state.update_sticks(left_x=left_x, left_y=left_y)
        
        # Right stick
        if 3 < num_axes:  # Right stick X
            right_x = self._joystick.get_axis(3)
            right_y = -self._joystick.get_axis(4) if 4 < num_axes else 0.0  # Invert Y
            self.controller_state.update_sticks(right_x=right_x, right_y=right_y)
    
    def _read_triggers(self):
        """Read trigger values."""
        num_axes = self._joystick.get_numaxes()
        
        # Handle different controller configurations
        if num_axes >= 6:
            # XInput style: separate LT/RT axes
            left_trigger = self._joystick.get_axis(2)   # Usually LT
            right_trigger = self._joystick.get_axis(5)  # Usually RT
        elif num_axes >= 3:
            # DirectInput style: combined axis
            combined = self._joystick.get_axis(2)
            # Split combined axis into separate triggers
            left_trigger = max(-1.0, -combined)
            right_trigger = max(-1.0, combined)
        else:
            left_trigger = right_trigger = -1.0
        
        self.controller_state.update_triggers(left_trigger, right_trigger)
    
    def _read_dpad(self):
        """Read D-pad state."""
        if self._joystick.get_numhats() > 0:
            hat_value = self._joystick.get_hat(0)
            direction = DPAD_MAPPING.get(hat_value, -1)
            self.controller_state.update_dpad(direction)
        else:
            self.controller_state.update_dpad(-1)
    
    def get_controller_info(self) -> dict:
        """Get information about connected controller."""
        if not self._joystick or not self.controller_state.connected:
            return {"connected": False}
        
        return {
            "connected": True,
            "name": self._joystick.get_name(),
            "num_buttons": self._joystick.get_numbuttons(),
            "num_axes": self._joystick.get_numaxes(),
            "num_hats": self._joystick.get_numhats()
        }