from PyQt5.QtCore import QThread, pyqtSignal, QTimer
from controller_state import ControllerState
from logger import Logger
from settings import TICK_HZ


class UIBridge(QThread):
    """Bridge between controller data and UI updates."""
    
    # Signals for UI updates
    ui_update_requested = pyqtSignal(dict)  # UI state data
    connection_changed = pyqtSignal(bool)   # Controller connection status
    
    def __init__(self, controller_state: ControllerState, logger: Logger):
        super().__init__()
        self.controller_state = controller_state
        self.logger = logger
        self.running = False
        
        # UI update timer
        self._ui_timer = None
        self._update_interval = int(1000 / TICK_HZ)  # Convert Hz to ms
        
        self.logger.info(f"UI Bridge initialized (update rate: {TICK_HZ}Hz)")
    
    def start_bridge(self):
        """Start the UI bridge."""
        if not self.running:
            self.running = True
            self.start()  # Start QThread
            self.logger.info("UI Bridge started")
    
    def stop_bridge(self):
        """Stop the UI bridge"""
        self.running = False
        self.quit()
        if self.isRunning():
            self.wait(1000)
        self.logger.info("UI Bridge stopped")
    
    def run(self):
        """Main thread loop for UI update"""
        self.logger.info("UI Bridge thread started")
        self._ui_timer = QTimer()
        self._ui_timer.setInterval(self._update_interval)
        self._ui_timer.timeout.connect(self._update_ui)
        self._ui_timer.start()
        self.exec_()
        if self._ui_timer:
            self._ui_timer.stop()
            self._ui_timer.deleteLater()
        self.logger.info("UI Bridge thread finished")
    
    def _update_ui(self):
        """Update UI with current controller state."""
        try:
            ui_state = self.controller_state.get_ui_state()
            self.ui_update_requested.emit(ui_state)
        except Exception as e:
            self.logger.failure(f"Failed to update UI: {e}")
    
    def force_ui_update(self):
        """Force an immediate UI update."""
        self._update_ui()