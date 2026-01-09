This application collects USB inputs from a XBOX series one generic controller, displays it and sends it via UDP.
This application is part of a project aiming towards piloting a car remotely and is the link between the user and the car.
The pipeline is the following : Controller inputs -> Application -> UDP formatted data -> ESP 32 -> STM32 -> Motors.

This application was developped under a Linux distribution using PyQt5.
To install :
Clone the repository containing the app : "git clone "https://gitlab.ecole.ensicaen.fr/rozoy/vroomvroomproject""
Go to the APPLI folder : "cd APPLI"
Install Python, venv and pip : "sudo apt install -y python3 python3-venv python3-pip"
Create a virtual environment : "python3 -m venv .venv"
Activate the venv : "source.venv/bin/activate"
(Optionnal) Update pip : "python -m pip install --upgrade pip"
Install dependancies : "pip install -r requirements.txt"

To run :
In the activated venv : "python3 main.py"
The html file should load and the UI should appear on screen.
Press "Start App" to start all threads, logs should appear both on the terminal and the UI to confirm initialization.
Connect your XBOX controller. Your inputs should be displayed and sent through UDP (logs only show 1/5 of the data sent to prevent flooding).
You can disconnect/reconnect your controller midst running without crashing.

To use with ESP 32 :
Make sure your laptop is connected to the same network as the ESP 32.
In settings.py, change ESP_32_HOST and ESP_32_PORT to your ESP 32 IP and desired port (usally 5500 for UDP).

Development :
The application was developped with a multi-threaded architecture using object oriented programming and QThread (main_window, gamepad_reader, ui_bridge and happening_handler) for reactivity in key features : Collecting inputs, displaying the inputs and processing the inputs. The html file exposes APIs used in the logger class for visual logging with timestamp. Logger is declared in main_window and should be added to the constructor (init) of any classes requiring logs. Logger exposes 3 APIs : success, failure, info. Main_window is the key thread, it connects signals and slots, handles the start and stop buttons. The bridge connects the UI to main_window using signals. Controller state is a datastruct, it represents a snapshot of the controller at POLL_HZ rate. Formatter converts the controller inputs to the desired format before UDP sending. Gamepad_reader uses pygame to return a snapshot of the controller state at given POLL_HZ rate. This snapshot is formatted then the ui_bridge uses it to update the UI at TICK_HZ rate and the udp_sender sends it at TICK_HZ rate. Happening_handler processes the inputs and returns events depending on the inputs.

Written by The Phong DOUANGMANIVONG.
