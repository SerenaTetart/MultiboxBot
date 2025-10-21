# MultiboxBot

## Table of contents
* [General infos](#general-infos)
* [Requirements and compilation](#requirements-and-compilation)

## General Infos

*The project is still under development*

MultiboxBot is an **in-process** bot for multiboxing on WoW 1.12.1 with up to 25 accounts using DLL injection, hooking and sockets. *(2.4.3 and 3.3.5 versions are planned)*

This project is the continuation of the **out-of-process** project <a href=https://github.com/Serenalyw/PixelBot>PixelBot</a>.

And this is an exemple of how the interface looks like using 2 monitors:

<p align="center">
<img src="https://user-images.githubusercontent.com/65224852/204255664-b6724d95-9df1-42df-8076-947da46f8220.png">
</p>

All windows are placed automatically by the program when the button **Launch** is pushed, if a window crash or move you can use the button **Repair** to replace it.

You can set the role and specialisation of every account by clicking on the menu next to the character's name:

<p align="center">
<img src="https://user-images.githubusercontent.com/65224852/204256381-dd4b8998-c72c-4ac3-91d8-ede0aff2101a.png">
</p>

*Melee characters will follow the tank of the current group and ranged will follow melee characters of the current group.*

Finally you can register every account credentials (password and username) on a .config file to connect to your accounts faster:

<p align="center">
<img src="https://user-images.githubusercontent.com/65224852/235278038-91fcf197-89e7-4a7e-a4b2-69a9d6fb96fe.png">
</p>

## Requirements and compilation

To use this project you'll need to compile the C++ code then paste **"Loader.dll"** and **"Bootstrap.exe"** into the **"test/src"** folder then execute **"start.bat"** that will take care of everything *(even the python part)*, to finish you have to add the WoW folder within the interface *(there is a button in the option tab)*.

In order to edit and compile the code I am using Visual studio 2022. There is no particular requirements, just use the latest version of C++. *(C++ 17)*
