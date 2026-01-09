import os
import json

# Qt imports
from PyQt5.QtWidgets import QMainWindow
from PyQt5.QtWebEngineWidgets import QWebEngineView
from PyQt5.QtWebChannel import QWebChannel
from PyQt5.QtCore import pyqtSlot, QUrl, Qt

# Local imports
from logger import Logger
from controller_state import ControllerState
from gamepad_reader import GamepadReader
from ui_bridge import UIBridge
from bridge import Bridge  # QObject exposed to the WebChannel (JS <-> Python)
from settings import UI_CONFIG
from udp_sender import UDPSender


class MainWindow(QMainWindow):
    """Main application window with HTML UI."""

    def __init__(self):
        super().__init__()

        # --- Core services ---
        self.logger = Logger(enable_console=True, enable_timestamps=True)

        # Shared state (thread-safe container for controller data)
        self.controller_state = ControllerState()
        self.controller_state.set_logger(self.logger)

        # Threads: gamepad reader (pygame) + UI cadence bridge + UDP sender
        self.gamepad_reader = GamepadReader(self.controller_state, self.logger)
        self.ui_bridge = UIBridge(self.controller_state, self.logger)
        self.udp_sender = UDPSender(self.controller_state, self.logger)

        # Web channel pieces (created once page is loaded)
        self.web_view = None
        self.channel = None
        self.bridge = None

        # UI setup and wiring
        self._init_ui()
        self._connect_signals()

        self.logger.success("Main window initialized")

    # -------------------------------------------------------------------------
    # UI bootstrap
    # -------------------------------------------------------------------------
    def _init_ui(self):
        """Initialize the web-based UI."""
        self.setWindowTitle("VROOM VROOM PROJECT")
        self.setGeometry(100, 100, 1400, 900)

        # Create web view
        self.web_view = QWebEngineView()
        self.setCentralWidget(self.web_view)

        # Load HTML file (local file path)
        html_path = os.path.join(os.path.dirname(__file__), "main_window.html")
        if os.path.exists(html_path):
            url = QUrl.fromLocalFile(os.path.abspath(html_path))
            self.web_view.load(url)
            self.logger.success(f"HTML UI loaded: {html_path}")
        else:
            self.logger.failure(f"HTML file not found: {html_path}")

    def _connect_signals(self):
        """Connect all signal/slot connections."""

        # Logger -> UI (append messages into the HTML console)
        self.logger.logMessage.connect(self.ui_append_log)

        # GamepadReader -> (optional) direct UI push (can be kept for fast feedback)
        self.gamepad_reader.connection_changed.connect(self._ui_set_controller_status, Qt.QueuedConnection)

        # UIBridge -> UI (status and periodic snapshots)
        self.ui_bridge.connection_changed.connect(self._ui_set_controller_status, Qt.QueuedConnection)
        self.ui_bridge.ui_update_requested.connect(self._ui_push_state, Qt.QueuedConnection)

        # Web page fully loaded
        self.web_view.loadFinished.connect(self._on_page_loaded)

        # UDP_sender -> UI
        self.udp_sender.connection_status_changed.connect(self._ui_set_udp_status)

    # -------------------------------------------------------------------------
    # Page loaded: inject config and build the WebChannel bridge
    # -------------------------------------------------------------------------
    def _on_page_loaded(self, success: bool):
        """Handle web page load completion."""
        if not success:
            self.logger.failure("Failed to load HTML UI")
            return

        self.logger.success("HTML UI loaded successfully")

        # Inject configuration into the page (as a plain JS object)
        self._inject_config()

        # Create QWebChannel and expose Python Bridge to JS (window.bridge)
        self._setup_webchannel()
        self._ui_set_controller_status(self.controller_state.connected)  # push initial
        self._ui_set_app_status("Running" if self.gamepad_reader.isRunning() else "Stopped")

    def _inject_config(self):
        """Inject configuration into the HTML page."""
        try:
            cfg = json.dumps(UI_CONFIG)  # ensure valid JS object
        except Exception:
            cfg = "{}"
        self._ui_js(f"window.CONFIG = {cfg};")
        self.logger.info("Configuration injected into HTML")

    def _setup_webchannel(self):
        """Create QWebChannel and expose Bridge as 'bridge' to JS.

        Note: Your HTML must include:
              <script src="qrc:///qtwebchannel/qwebchannel.js"></script>
              so that 'QWebChannel' is available on the page.
        """
        # Python side objects
        if not self.bridge:
            self.bridge = Bridge(logger=self.logger)
            # JS -> PY
            self.bridge.startRequested.connect(self._on_start_clicked)
            self.bridge.stopRequested.connect(self._on_stop_clicked)

        if not self.channel:
            self.channel = QWebChannel(self.web_view.page())
            self.channel.registerObject("bridge", self.bridge)
            self.web_view.page().setWebChannel(self.channel)

        # JS side binding (if qwebchannel.js is present in the page)
        bind_js = """
        (function(){
          if (typeof QWebChannel === 'undefined' || !qt || !qt.webChannelTransport) {
            console.warn('QWebChannel not available; please include qwebchannel.js');
            return;
          }
          new QWebChannel(qt.webChannelTransport, function(channel){
            window.bridge = channel.objects.bridge;
            if (window.UI && window.UI.onBridgeReady) window.UI.onBridgeReady();
          });
        })();
        """
        self._ui_js(bind_js)

    # -------------------------------------------------------------------------
    # Small helpers to call page JS safely (and tolerate different API names)
    # -------------------------------------------------------------------------
    def _ui_js(self, code: str):
        """Run arbitrary JS in the page."""
        self.web_view.page().runJavaScript(code)

    def _ui_set_app_status(self, status: str):
        running = "true" if str(status).lower() == "running" else "false"
        self._ui_js(f"""
            (function(r){{
            if (window.UI && typeof UI.setAppState === 'function') {{
                UI.setAppState(r);
            }}
            var s = document.getElementById('appStatus');
            var d = document.getElementById('appDot');
            if (s) s.textContent = r ? 'Running' : 'Stopped';
            if (d) d.className = 'status-dot ' + (r ? 'dot-running' : 'dot-stopped');
        }})({running});
        """)

    def _ui_set_controller_status(self, connected: bool):
        js_bool = "true" if connected else "false"
        self._ui_js(f"""
            (function(c){{
            if (window.UI && typeof UI.setControllerConnected === 'function') {{
                UI.setControllerConnected(c);
            }}
            var n = document.getElementById('ctlStatus');
            if (n) n.textContent = c ? 'Connected' : 'Not connected';
        }})({js_bool});
        """)

    def _ui_push_state(self, state: dict):
        """Push a full controller snapshot to the UI (axes, buttons, triggers)."""
        payload = json.dumps(state)
        self._ui_js(
            f'(window.UI && window.UI.updateGamepad && window.UI.updateGamepad({payload})) || '
            f'(window.UI && window.UI.updateControlState && window.UI.updateControlState({payload})) || '
            f'(window.ui && window.ui.updateGamepad && window.ui.updateGamepad({payload})) || '
            f'(window.ui && window.ui.updateControlState && window.ui.updateControlState({payload}));'
        )

    def _ui_set_udp_status(self, status: str):
        """Update visual status on UI"""
        safe = "connected" if status == "connected" else "disconnected"
        self._ui_js(f"""
            (function(s){{
            var n = document.getElementById('udpStatus');
            if (n) n.textContent = s;
        }})("{safe}");
        """)

    # -------------------------------------------------------------------------
    # Start/Stop coming from JS (via Bridge)
    # -------------------------------------------------------------------------
    def _on_start_clicked(self):
        """Start threads from UI action (idempotent)."""
        # Start gamepad reader thread
        if not self.gamepad_reader.isRunning():
            self.gamepad_reader.start_reading()

        # Start UI bridge thread (periodic UI updates)
        if not self.ui_bridge.isRunning():
            self.ui_bridge.start_bridge()

        # Start sender thread
        if not self.udp_sender.isRunning():
            self.udp_sender.start_sending()

        # Feedback + UI state
        self.logger.success("Application started from UI")
        self._ui_set_app_status("Running")

    def _on_stop_clicked(self):
        """Stop threads from UI action (idempotent)."""
        # Stop reader
        try:
            if self.gamepad_reader.isRunning():
                self.gamepad_reader.stop_reading()
                self.gamepad_reader.wait(1000)
        except Exception as e:
            self.logger.failure(f"Stop reader error: {e}")

        # Stop UI bridge
        try:
            if self.ui_bridge.isRunning():
                self.ui_bridge.stop_bridge()
        except Exception as e:
            self.logger.failure(f"Stop bridge error: {e}")

        # Stop sender
        try:
            if self.udp_sender.isRunning():
                self.udp_sender.stop_sending()
        except Exception as e:
            self.logger.failure(f"Stop UDP error: {e}")

        # Feedback + UI state
        self.logger.info("Application stopped from UI")
        self._ui_set_app_status("Stopped")

    # -------------------------------------------------------------------------
    # Slots used by logger and legacy direct pushes
    # -------------------------------------------------------------------------
    @pyqtSlot(str)
    def ui_append_log(self, message: str):
        """Append message to the UI log console."""
        # Escape for safe JS string literal
        escaped = (
            message.replace("\\", "\\\\")
                   .replace("'", "\\'")
                   .replace("\n", "\\n")
        )
        self._ui_js(f"if (window.UI) window.UI.appendLog('{escaped}');"
                    f"if (window.ui) window.ui.appendLog && window.ui.appendLog('{escaped}');")

    # -------------------------------------------------------------------------
    # Programmatic start/stop (used on app open/close)
    # -------------------------------------------------------------------------
    def start_app(self):
        """Start the gamepad reading and UI updates."""
        try:
            self.gamepad_reader.start_reading()
            self.ui_bridge.start_bridge()
            self.ui_set_app_state(True)
            self.logger.success("Application started")
        except Exception as e:
            self.logger.failure(f"Failed to start application: {e}")

    def stop_app(self):
        """Stop the gamepad reading and UI updates."""
        try:
            self.gamepad_reader.stop_reading()
            self.ui_bridge.stop_bridge()
            self.ui_set_app_state(False)
            self.logger.success("Application stopped")
        except Exception as e:
            self.logger.failure(f"Failed to stop application: {e}")

    # -------------------------------------------------------------------------
    # Window close: stop threads cleanly
    # -------------------------------------------------------------------------
    def closeEvent(self, event):
        """Handle window close event."""
        self.logger.info("Closing application...")
        self.stop_app()
        event.accept()
