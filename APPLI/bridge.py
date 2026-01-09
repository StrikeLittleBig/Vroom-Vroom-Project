from PyQt5.QtCore import QObject, pyqtSignal, pyqtSlot

class Bridge(QObject):
    startRequested = pyqtSignal()
    stopRequested  = pyqtSignal()

    def __init__(self, logger, parent=None):
        super().__init__(parent)
        self.logger = logger

    @pyqtSlot()
    def start_app(self):
        self.logger.info("Start requested")
        self.startRequested.emit()

    @pyqtSlot()
    def stop_app(self):
        self.logger.info("Stop requested")
        self.stopRequested.emit()

    @pyqtSlot(str)
    def debugPing(self, msg):
        self.logger.info(f"JS->PY ping: {msg}")
