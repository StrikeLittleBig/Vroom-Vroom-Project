from PyQt5.QtCore import QObject, pyqtSignal
import datetime
import sys
from typing import Optional

class Logger(QObject):
    """
    Thread-safe logger that outputs to console and emits Qt signals for UI integration.
    
    Usage:
        logger = Logger()
        logger.logMessage.connect(main_window.ui_append_log)
        logger.success("Operation completed")
        logger.failure("Connection failed")
        logger.info("Status update")
    """
    
    # Qt signal for UI integration - connects to HTML appendLog API
    logMessage = pyqtSignal(str)
    
    def __init__(self, 
                 enable_console: bool = True, 
                 enable_timestamps: bool = True,
                 component_name: Optional[str] = None):
        """
        Initialize logger.
        
        Args:
            enable_console: If True, also print to console
            enable_timestamps: If True, include timestamps in messages
            component_name: Optional component identifier for log messages
        """
        super().__init__()
        self.enable_console = enable_console
        self.enable_timestamps = enable_timestamps
        self.component_name = component_name
    
    def success(self, message: str):
        """
        Log a success message.
        
        Args:
            message: The success message to log
        """
        self._log("SUCCESS", message)
    
    def failure(self, message: str):
        """
        Log a failure/error message.
        
        Args:
            message: The failure message to log
        """
        self._log("FAILURE", message)
    
    def info(self, message: str):
        """
        Log an informational message.
        
        Args:
            message: The info message to log
        """
        self._log("INFO", message)
    
    def _log(self, level: str, message: str):
        """
        Internal logging method that formats and emits the message.
        
        Args:
            level: Log level (SUCCESS, FAILURE, INFO)
            message: The message to log
        """
        # Build formatted message
        formatted_message = self._format_message(level, message)
        
        # Emit Qt signal for UI (thread-safe)
        self.logMessage.emit(formatted_message)
        
        # Console output if enabled
        if self.enable_console:
            self._console_output(level, formatted_message)
    
    def _format_message(self, level: str, message: str) -> str:
        """
        Format the log message with timestamp and component info.
        
        Args:
            level: Log level
            message: Raw message
            
        Returns:
            Formatted message string
        """
        parts = []
        
        # Timestamp
        if self.enable_timestamps:
            timestamp = datetime.datetime.now().strftime("%H:%M:%S")
            parts.append(f"[{timestamp}]")
        
        # Log level
        parts.append(level)
        
        # Component name
        if self.component_name:
            parts.append(f"[{self.component_name}]")
        
        # Message
        parts.append(f"— {message}")
        
        return " ".join(parts)
    
    def _console_output(self, level: str, formatted_message: str):
        """
        Output to console with optional ANSI colors.
        
        Args:
            level: Log level for color selection
            formatted_message: Already formatted message
        """
        # ANSI color codes for different log levels
        COLORS = {
            "SUCCESS": "\033[92m",  # Green
            "FAILURE": "\033[91m",  # Red  
            "INFO": "\033[96m",     # Cyan
        }
        RESET = "\033[0m"
        
        # Apply color if available
        color = COLORS.get(level, "")
        colored_message = f"{color}{formatted_message}{RESET}" if color else formatted_message
        
        # Output to appropriate stream
        output_stream = sys.stderr if level == "FAILURE" else sys.stdout
        print(colored_message, file=output_stream, flush=True)