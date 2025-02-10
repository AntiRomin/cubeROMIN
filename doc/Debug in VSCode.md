# Debug in VSCode

- Openocd requied
- VScode requied

## Installing Openocd
Please install the Openocd manually. You can download the openocd at here: https://github.com/xpack-dev-tools/openocd-xpack/releases. Remember to download the "xpack-openocd-version-win32-x64.zip" for windows. I highly recommend you add the openocd to your enviroment path. Otherwise you will nedd to use absolute addresses when writing vscode script.
>/your openocd install path/bin

Enter the following script in your cmd:
>openocd -v

You can see openocd version information

## Installing plug-in for VSCode
You need to install "Cortex-Debug" for VSCode. You can find in Extensions when you type "Cortex-Debug" in search bar.

## Write c_cpp_properties.json for C language support
We should change the intelliSensMode to "windows-gcc-arm" for arm style C language typing. Also we should add GNU ARM Toolchain's c lib to includePath for introducing header file. A simple version is following:

```
{
    "env": {
        "gnuArmIncludePath": [
            "your gnu arm toolchain install path/lib/gcc/arm-none-eabi/10.3.1/include",
            "your gnu arm toolchain install path/lib/gcc/arm-none-eabi/10.3.1/include-fixed"
        ],
        "gnuArmCompilerPath": [
            "your gnu arm toolchain install path/bin/arm-none-eabi-gcc.exe"
        ]
    },
    "configurations": [
        {
            "name": "GNU Arm",
            "intelliSenseMode": "windows-gcc-arm",
            "includePath": [
                "${gnuArmIncludePath}",
                "${workspaceFolder}",
                "./lib/main/CMSIS/Core/Include",
                "./lib/main/CMSIS/DSP/Include",
                "./lib/main/STM32H7/Drivers/CMSIS/Device/ST/STM32H7xx/Include",
                "./lib/main/STM32H7/Drivers/STM32H7xx_HAL_Driver/Inc",
                "./lib/main/STM32H7/Middlewares/ST/STM32_USB_Device_Library/Core/Inc",
                "./lib/main/STM32H7/Middlewares/ST/STM32_USB_Host_Library/Core/Inc",
                "./src/main",
                "./src/main/target",
                "./src/main/startup",
                "./src/main/drivers/stm32"
            ],
            "defines": [
                "DEBUG",
                "STM32H743xx"
            ],
            "compilerPath": "${gnuArmCompilerPath}"
        }

    ],
    "version": 4
}
```

## Write tasks.json for VSCode
```
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "clean",
            "type": "shell",
            "detail": "clean the current target",
            "command": "make clean",
            "problemMatcher": "$gcc"
        },
        {
            "label": "make release",
            "type": "shell",
            "detail": "Build the target",
            "command": "make",
            "problemMatcher": "$gcc"
        },
        {
            "label": "make debug",
            "type": "shell",
            "detail": "Build with DEBUG=GDB",
            "command": "make",
            "args": [
                "DEBUG=GDB"
            ],
            "problemMatcher": "$gcc"
        },
        {
            "label": "erase",
            "type": "shell",
            "detail": "erase full flash",
            "command": "openocd",
            "args": [
                "-f",
                "interface/stlink.cfg",
                "-f",
                "target/stm32h7x_dual_bank.cfg",
                "-c",
                "flash init; init; reset halt; flash erase_sector 0 0 last; flash erase_sector 1 0 last; exit"
            ],
            "problemMatcher": "$gcc"
        },
        {
            "label": "download with openocd connect",
            "type": "shell",
            "detail": "download elf to mcu",
            "command": "openocd",
            "args": [
                "-f",
                "interface/stlink.cfg",
                "-f",
                "target/stm32h7x_dual_bank.cfg",
                "-c",
                "program ./obj/debug/debug.elf verify reset exit"
            ],
            "problemMatcher": "$gcc"
        },
        {
            "label": "debug with download",
            "type": "shell",
            "dependsOn": [
                "make debug",
            ],
            "problemMatcher": "$gcc"
        }
    ]
}
```

## Write launch.json for Debug
```
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug with OpenOCD",
            "type": "cortex-debug",
            "request": "launch",
            "executable": "./obj/debug/debug.elf",
            "cwd": "${workspaceRoot}",
            "servertype": "openocd",
            "configFiles": [
                "interface/stlink.cfg",
                "target/stm32h7x.cfg"
            ],
            "runToEntryPoint": "SystemInit",
            "showDevDebugOutput": "none",

            "preLaunchTask": "debug with download"
        }
    ]
}
```