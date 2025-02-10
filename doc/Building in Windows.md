# Building in windows

- Chocolatey's make requied
- Git's mingw64 enviroment requied
- GNU ARM Toolchain requied

## Installing Chocolatey and requied package
Run this following script in your cmd to install chocolatey.
>Set-ExecutionPolicy Bypass -Scope Process -Force; iex ((New-Object System.Net.WebClient).DownloadString('http://internal/odata/repo/ChocolateyInstall.ps1'))

Run this following script in your cmd to install make by choco.
>choco install make

## Added git's mingw64 enviroment to your enviroment path
Git have some linux shell type command for git bash. We can use it for writing linux shell command in our makefile. Please at this to your enviroment path.
>/your git install path/usr/bin

Enter the following script in your cmd:
>make -v

If you can see GNU Make version information, then congratulations, you have been successful up to this step.

## Setup GNU ARM Toolchain
To compile this project source code for the ARM processors, the GNU ARM Toolchain is required.Please install the GNU ARM Toolchain manually. You can download the toolchain at here: https://developer.arm.com/downloads/-/gnu-rm.
After you installed the GNU ARM Toolchain, I highly recommend you add the toolchain to your enviroment path. Otherwise you will need to use absolute addresses when writing the makefile.
>/your GNU ARM Toolchain install path/bin

Enter the following script in your cmd:
>arm-none-eabi-gcc -v

If you can see gcc version information, then you can begin to compile this project source code.