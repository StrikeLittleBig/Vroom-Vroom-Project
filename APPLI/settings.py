# === Application Configuration ===

# Network settings
ESP32_HOST = "192.168.4.1"
ESP32_PORT = 5500

# Performance settings
TICK_HZ = 100        # UI update rate (10ms)
POLL_HZ = 180        # Gamepad polling rate

# === Controller Mapping ===

# Pygame button indices → logical button names
PYGAME_BUTTON_MAPPING = {
    0: "A",
    1: "B", 
    2: "X",
    3: "Y",
    4: "LB",
    5: "RB",
    6: "Share",    # Back/View button
    7: "Select",   # Start/Menu button
    8: "Xbox",     # Guide button (may not exist on all systems)
    9: "LS",       # Left stick click
    10: "RS",      # Right stick click
}

# Pygame axis indices
PYGAME_AXIS_MAPPING = {
    0: "left_stick_x",
    1: "left_stick_y",     # Will be inverted in code
    2: "left_trigger",     # May be combined LT-RT axis on some systems
    3: "right_stick_x", 
    4: "right_stick_y",    # Will be inverted in code
    5: "right_trigger",    # Separate RT axis (XInput)
}

# D-pad mapping (pygame hat values → direction codes)
DPAD_MAPPING = {
    (0, 1): 0,   # Up
    (1, 0): 1,   # Right  
    (0, -1): 2,  # Down
    (-1, 0): 3,  # Left
    (0, 0): -1,  # Neutral
}

# === UDP Protocol Mapping ===

# Logical button names → UDP bit positions (according to your 28-byte spec)
UDP_BUTTON_MAPPING = {
    "X": 0,
    "Y": 1, 
    "A": 2,
    "B": 3,
    "LB": 4,
    "RB": 5,
    "Share": 6,
    "Select": 7,
    "Xbox": 8,
    "LS": 13,      # Left stick click
    "RS": 14,      # Right stick click
}

# D-pad direction codes → UDP button names
UDP_DPAD_MAPPING = {
    0: "DpadUp",     # bit 9
    1: "DpadRight",  # bit 12
    2: "DpadDown",   # bit 10
    3: "DpadLeft",   # bit 11
}

# Buttons → bit position
UDP_BUTTON_BITS = {
    "X": 0,
    "Y": 1,
    "A": 2,
    "B": 3,
    "LB": 4,
    "RB": 5,
    "Share": 6,
    "Select": 7,
    "Xbox": 8,
    "DpadUp": 9,
    "DpadDown": 10,
    "DpadLeft": 11,
    "DpadRight": 12,
    "LS": 13,   # Left stick click
    "RS": 14,   # Right stick click
    # 15..31: 0
}

# === UI Display Mapping ===

# Logical button names → UI bit positions (for HTML compatibility)
UI_BUTTON_MAPPING = {
    "A": 0,
    "B": 1,
    "X": 2, 
    "Y": 3,
    "LB": 4,
    "RB": 5,
    "Share": 6,
    "Select": 7,
    "LS": 8,
    "RS": 9,
    "Xbox": 10,
}

# === Data Format Settings ===

# Input normalization ranges
AXIS_RANGE = (-1.0, 1.0)       # Stick axes normalized range
TRIGGER_RANGE = (-1.0, 1.0)    # Trigger normalized range (-1=released, 1=pressed)
DEADZONE = 0.06                 # Deadzone for stick drift

# UDP packet format (28 bytes total)
UDP_PACKET_FORMAT = {
    "struct": "<I6f",           # Little-endian: uint32 + 6 float32
    "size": 28,                 # Total packet size in bytes
    "fields": [                 # Field order in packet
        "buttons",              # uint32 (4 bytes)
        "left_stick_x",         # float32 (4 bytes)  
        "left_stick_y",         # float32 (4 bytes)
        "right_stick_x",        # float32 (4 bytes)
        "right_stick_y",        # float32 (4 bytes)
        "left_trigger",         # float32 (4 bytes)
        "right_trigger",        # float32 (4 bytes)
    ]
}

# UI configuration for injection into HTML
UI_CONFIG = {
    "buttonMapping": UI_BUTTON_MAPPING,
    "triggerRange": TRIGGER_RANGE,
    "axisRange": AXIS_RANGE,
    "version": "2.0"
}