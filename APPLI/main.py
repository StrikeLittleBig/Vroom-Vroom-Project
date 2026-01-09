import sys
from PyQt5.QtWidgets import QApplication
from PyQt5.QtCore import Qt
from main_window import MainWindow
from PyQt5.QtCore import QCoreApplication, Qt


def main():
    """Main application entry point."""
    QCoreApplication.setAttribute(Qt.AA_EnableHighDpiScaling, True)
    # Create QApplication
    app = QApplication(sys.argv)
    app.setApplicationName("VROOM VROOM PROJECT")
    app.setOrganizationName("VroomVroom")
    
    # Enable high DPI scaling
    app.setAttribute(Qt.AA_EnableHighDpiScaling, True)
    app.setAttribute(Qt.AA_UseHighDpiPixmaps, True)
    
    # Create and show main window
    main_window = MainWindow()
    main_window.show()
    
    # Run the application
    try:
        sys.exit(app.exec_())
    except KeyboardInterrupt:
        print("\nApplication interrupted by user")
        main_window.stop_app()
        sys.exit(0)


if __name__ == "__main__":
    main()