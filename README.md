# Note: This repository does not reflect the latest LANDrop releases. We decided to temporarily close source LANDrop and we might re-open source it in the future. Thanks for your understanding!

<img src="LANDrop/icons/banner.png" width="300">

![Package](https://github.com/LANDrop/LANDrop/workflows/Package/badge.svg)

> Drop any files to any devices on your LAN. No need to use instant messaging for that anymore.

LANDrop is a cross-platform tool that you can use to conveniently transfer photos, videos, and other types of files to other devices on the same local network.

You can download prebuilts of LANDrop from the [official website](https://landrop.app/#downloads).

## Features

- Cross platform: when we say it, we mean it. iOS, Android, macOS, Windows, Linux, name yours.
- Ultra fast: uses your local network for transferring. Internet speed is not a limit.
- Easy to use: intuitive UI. You know how to use it when you see it.
- Secure: uses state-of-the-art cryptography algorithm. No one else can see your files.
- No cellular data: outside? No problem. LANDrop can work on your personal hotspot, without consuming cellular data.
- No compression: doesn't compress your photos and videos when sending.

## Building

The AppImage we provide as the prebuilt for Linux might not work on your machine. You can build LANDrop by yourself if the prebuilt doesn't work for you.

To build LANDrop:

1. Download and install the dependencies: [Qt](https://www.qt.io/download-qt-installer) and [libsodium](https://libsodium.gitbook.io/doc/#downloading-libsodium)  
    If you are using a Debian-based distro, such as Ubuntu, you can install libsodium via
    ```
    sudo apt install libsodium-dev
    ```
2. Clone this repository
    ```
    git clone https://github.com/LANDrop/LANDrop
    ```
3. Run the following commands
    ```
    mkdir -p LANDrop/build
    cd LANDrop/build
    qmake ../LANDrop
    make -j$(nproc)
    sudo make install
    ```
4. You can now run LANDrop via
    ```
    landrop
    ```
