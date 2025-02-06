# dpset

**dpset** is a small **Qt** application for visually arranging and configuring monitors on Linux using **xrandr** and **xinput**.  
It provides an intuitive drag-and-drop interface to set positions, resolutions, orientations, and primary monitors.  
The application can also generate a shell script to make monitor setups easily reproducible on reboot.

> **Note**: dpset is **not** part of the official Sweelinq software. It is an independent utility.

---

## Table of Contents
- [Features](#features)
- [Requirements](#requirements)
- [Building](#building)
- [Running](#running)
- [Usage](#usage)
- [Notes](#notes)

---

## Features
- :computer: **Visual Monitor Layout**  
  Drag and drop monitors in a 2D scene. They snap to each other when close.
- :gear: **Context Menu**  
  - **Identify**: Temporarily shows an overlay on the physical screen.  
  - **Primary**: Mark a specific monitor as the primary display.  
  - **Resolution**: Select from known resolutions or set a custom resolution.  
  - **Orientation**: Rotate the display (normal, left, right, inverted).  
  - **Touch Device Mapping**: Map a detected **xinput** device to a particular monitor.
    
  <img width="799" alt="screenshot" src="https://github.com/user-attachments/assets/e714c7bb-6679-46bb-be0e-bcaf0743123f" />
  
- :arrow_double_up: **Apply or Save**  
  - **Apply**: Immediately apply xrandr/xinput changes (non-persistent).  
  - **Script**: Save the configuration as a shell script for easy replication at startup.

---

## Requirements

1. **Qt 6.5** (Core, Gui, Widgets) or higher  
2. **CMake 3.19** or higher  
3. **xrandr** and **xinput** command-line tools must be installed and in your PATH  
4. A standard C++ compiler (e.g., g++ or clang)

---

## Building

1. **Clone** this repository:
```bash
git clone https://github.com/Sweelinq/dpset.git
```
2. **Create a build directory** and run CMake:
```bash
cd dpset mkdir build && cd build cmake ..
```
This will generate the build system (e.g., Makefiles, Ninja, etc.) based on your environment.

3. **Build** the project:
```bash
cmake --build .
```
After a successful build, you should have an executable named **dpset**.

---

## Running

You have two options to run **dpset**:

1. **Download the latest release as an AppImage**  
- Locate the `.appimage` file you downloaded (e.g., `dpset.AppImage`).  
- Make it executable:
  ```
  chmod +x dpset.AppImage
  ```
- Launch it:
  ```
  ./dpset.AppImage
  ```
2. **Use your own build**  
- From the **build** directory (or wherever you built the executable), run:
  ```
  ./dpset
  ```

If **xrandr** detects monitors, they appear as draggable items.  
If no monitors are detected, an error is shown.

---

## Usage

1. **Arrange Monitors**  
- Drag monitors around the scene to match your desired physical layout.  
- Monitors automatically snap together.
2. **Context Menu** (Right-click on a monitor)  
- **Identify**: Displays a large label on that actual monitor for easy identification.  
- **Primary**: Toggle whether this monitor is primary.  
- **Resolution**: Choose from detected resolutions or set a custom resolution.  
- **Orientation**: Rotate the monitor (normal, left, right, inverted).  
- **Touch device**: Map a specific touch device (detected by xinput) to this monitor.
3. **Apply**  
- Click **Apply** to immediately run the necessary `xrandr` and `xinput` commands.
4. **Script**  
- Click **Script** to generate a shell script containing all relevant commands.  
- Run this script manually or integrate it into your startup routine (e.g., `~/.profile`, `~/.xinitrc`, or desktop environment services) to restore the layout after reboot.

An example of a generated script:
```bash
#!/bin/bash

xrandr --output HDMI-2 --mode 1600x720 --pos 3840x1440 --rotate normal --output HDMI-1 --mode 3840x2160 --pos 0x0 --rotate normal

find_xinput_device() {
    local target_id_path="$(echo "$1" | xargs)"
    local target_name="$(echo "$2" | xargs)"
    for id in $(xinput list --id-only); do
        local node_line
        node_line=$(xinput list-props "$id" 2>/dev/null | grep "Device Node")
        if [ -z "$node_line" ]; then
            continue
        fi
        local device_node
        device_node=$(echo "$node_line" | sed -n 's/.*"\(\/dev\/input\/[^\"]*\)".*/\1/p')
        if [ -z "$device_node" ]; then
            continue
        fi
        local udev_line
        udev_line=$(udevadm info "$device_node" 2>/dev/null | grep "E: ID_PATH=")
        if [ -z "$udev_line" ]; then
            continue
        fi
        local id_path_value
        id_path_value=$(echo "$udev_line" | cut -d'=' -f2 | xargs)
        if [ "$id_path_value" = "$target_id_path" ]; then
            local dev_name
            dev_name=$(xinput list --name-only "$id" | xargs)
            if [ "$dev_name" = "$target_name" ]; then
                echo "$id"
                return 0
            fi
        fi
    done
    echo "-1"
    return 1
}

#Touch mapping for HDMI-2
DEVICE_ID=$(find_xinput_device "pci-0000:00:14.0-usb-0:1:1.0" "Waveshare  Waveshare")
if [ "$DEVICE_ID" -eq "-1" ]; then
    echo "DEBUG: Could not find touch device for HDMI-2 with id_path pci-0000:00:14.0-usb-0:1:1.0 and name Waveshare  Waveshare" >&2
else
    xinput set-prop $DEVICE_ID 'Coordinate Transformation Matrix' 0.29411765 0 0.70588235 0 0.33333333 0.66666667 0 0 1
fi
```

---

## Notes

- **Non-persistent**  
The changes applied with **Apply** are not saved permanently. You must reapply them or run the generated script after each reboot.
- **Wayland**  
Currently relies on **xrandr** and **xinput** (X11); it does not support Wayland sessions.

---
