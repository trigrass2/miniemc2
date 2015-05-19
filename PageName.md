# Introduction #
Miniemc2 it's port of well known EMC2 CNC controller to embedded device. It allows you to control your CNC machine without PC via Ethernet or WI-FI (in the future). Miniemc2 provides STEP and DIRECTION signals to control stepper motors. Main features:
  * Up to 6 axis ( should be clarified during tests )
  * Step frequency up to 40 kHz
  * STEP's width and polarity fixed: 10 uS, rising
  * 19 digital inputs, 19 digital outputs. Each of them can be inverted.
  * Each output can be assigned to STEP/DIR/PWM or Digital Output ( controlled from NC program)
  * Each input can be assigned to axes Home or Limit switch, or to E-STOP button
  * 2 PWM channels with fixed frequency (1 kHz) and 100 values of duty cycle
  * NC program execution from USB Flash or SD card
  * FTP access for data uploading
  * Control over network with TkEMC
  * Original WEB interface instead of TkEMC
  * External control panel can be connected via Digital Inputs ( with _halui_ )

# Difference from original EMC2 #
As a base for miniEMC2 was selected version 2.2.0. Later was added changes from v 2.2.8, so this is almost EMC2-2.2.8. Due to S3C2440 CPU limitation was added some changes to HAL. First of all Xenomai real-time development framework is used instead of RTAI. There is no special abstraction for Xenomai, just added some changes to sim RTAPI abstraction in such way, that instead of _libpth_ used Xenomai's native API. Second diffrence is how hard realtime parts implemented. All standart HW drivers, like _partport_ and _stepgen_ was discarded due to their performance. Hard realtime functions, based on FIQ interrupt from periodic timer with 10 uS period, was used instead. This patch was taken from OpenMoko projects.  More details will be provided in the last part on this doc. At present time just one important note. _There is dedicated coordinate FIFO between motion controller and FIQ frequency synthesizer. That is we always have some delays between commanded and actual position. This delay can be calculated as N\*1mS, N is FIFO deep. By default FIFO deep is 32, so delay is 32 mS. User can vary this value from 8 to 32.
FIFO's delay adds some restrictions of usage of this port, please note this._
Interaction between _FIQ_ and _motion controller_ provided by dedicated HAL driver named **modminiemc**. It implements almost all functions of _parport_ and _stepgen_ components. All other HW drivers are absent at present time, but HAL components, like logical primitives, regulators, i.е. HW-independent parts, are avaliable to user.  miniEMC2 don't use mini2440's display due to low performance of GUI together with EMC2, only remote control is possible. User can choise between remote control by TkEMC and dedicated WEB-based GUI. Number of simultaneously working axis is open question. FIQ frequency synthesizer supports up to 6 channels, but author not sure if CPU has enough performance. 4 channels forks fine, if you would like more - try and share results.
And the latest: _only trivial kinematics are working fine_, non-trivial ones also present, but they don't work due to CPU's perfomance. If you have read this chapter careful and you still interested in this project - go ahead with quick start in the next chapter.

# Quick start #

Software binary package consits tree parts: u-boot boot loader, Linux kernel image and root filesystem image (and its tarbal). Before start you need to dowload the latest firmware archive from _Download_ section of this site and untar it. Of course you need _mini2440_ evalution board without display. You need to connect it to PC with RS-232 cable and Ethernet cable. Will be good to have inseted SD card for NC program storing.

From PC side you have to install serial communication util, like PuTTY and TFTP server, for example TFTP32 for Windows or any other. Copy files from the tarball to the root folder of TFTP server. Your PC should be connected to the same Ethernet network as the _mini2440_.

First you need to install u-boot bootloader. I just point you to page with detailed description of  [setup process](http://www.google.com/url?url=http://wiki.linuxmce.org/index.php/Mini2440%23Flashing_uboot&rct=j&q=mini2440+u-boot+setup&usg=AFQjCNGKmz5THZr0yw7V5xiD08asalhM6Q&sa=X&ei=XOK2Tp2EIuP34QTalqnJBQ&ved=0CCYQygQwAQ&cad=rja). Binary image of u-boot provided by me is almost the same as in that one at my link except some additional environment varibles I've added. So if you have already flashed u-boot, then you can just set next variables under u-boot's serial console:
```

set update_kernel 'tftpboot 0x32000000 uImage; nand erase 0x60000 0x500000; nand write.e 0x32000000 0x60000 0x500000'
set update_fs 'tftpboot 0x32000000 rootfs.jffs2; nand erase 0x560000 0x2000000; nand write.e 0x32000000 0x560000 ${filesize}'
set bootargs 'console=ttySAC0,115200n8 root=/dev/mtdblock3 rootfstype=jffs2'
set bootcmd 'nand read.e 0x32000000 0x60000 0x500000; bootm 0x32000000'
set update 'run update_kernel;run update_fs;run bootcmd'
saveenv
```
Next you have to setup _mini2440_ IP address and your TFTP server IP:
```

set ipaddr 192.168.1.80
set serverip 192.168.1.8
run update
```
Change _192.168.1.8_  with the proper TFTP server address.
Last line starts firmware downloading and NAND flash update. After that it will start miniEMC2 booting process. First boot will take minute or two. WEB server will be accessible at address _192.168.1.80_. Also device is accessible through FTP and SSH: _ftp://root:root@192.168.1.80_ ( that is user name **root**, password **root**). SD card and USB flash should be mounted and unmounted automatically. They are accessible at _/media/mmc_ and _/media/usb_. _Note: supported mounting of only one (last)  partition of SD or USB stick_. Filesystem type can be FAT16/32 or EXT3.

All images was tested on mini2440 with 256Mb and 1Gb of NAND flash.

# WEB interface #
miniEMC2 uses fully static WEB-page which loaded only once at first connect. After that integrated scripts are using AJAX requests to update displayed information. Such design provides some benefits for users: GUI works fast and there is no problem to customize HTML page for user's requirements - used only 3 files - index.html, css/webemc.css and js/miniemc2.js ( and jQuery and some plugins, but it's standard things).

WEB server is ready to access at address htpp://192.168.1.80 in 1 minute after start. You will see next page
![http://miniemc2.googlecode.com/files/miniemc2_3.png](http://miniemc2.googlecode.com/files/miniemc2_3.png)

It divided into 4 parts:
## Machine position panel ##
It's left panel of the page. As you can see there displayed _Axes position_, current _Feedrate_, slider for _Feedrate override_ and _E-Stop_ button. This panel always present on display independent on operation mode or selected Control panel. _In this version axis's position always displayed in **mm** independent on NC program unit._ Note: don't drag feedrate slider, just click bellow or above it. _E-Stop_ button has three modes: _normal_ when motors powers is off, _pressed_ ( red cross inside button ) when machine is powered and/or it is working. Click on this button immediately stops machine movement and releases motors. And third E-Stop state is "red" when external E-Stop button is pressed ( if such button exists in your configuration ).

## Machine control panel ##
It's the right panel of the page. Provided 4 types of panels and user can switch between them clicking on _Menu bar_.
### Manual control ###
This is default panel at WEB session start ( see pic. above). Here user can move axis pressing corresponding buttons. For **X** and **Y** axes here provided dedicated buttons - looks like joystick. Inside this "joystick" placed toggle button for spindle control. All other axes can be jogged by round buttons, movement will be performed on that axis, which was selected by _Select axis_ select box. Another select box with name _Jogging mode_ allows you to select 9 jogging modes: _cont._ - continus, that is, axis is moving till you press corresponding button and 500,100,10,1,0.1,0.01,0.001 allows to move axis to discrete distance.

### Homes ###
![http://miniemc2.googlecode.com/files/miniemc2_2.png](http://miniemc2.googlecode.com/files/miniemc2_2.png)
Provides functions for machine initial setup. On the left panel with "home" icon placed the buttons which perform "homming" sequence when user clicks them. This sequence dependent on INI file options ( see "EMC2 Integrator manual" for details).

Next subpanel( from top to bottom ) provides tools for select and setup current coordinate system.
Next subpanel display offsets of current coordinate system from absolute machine position.
Next subpanel displays active modal codes. And the last subpanel _Position control_ provide three buttons. _"Go home"_ it's shortcut for MDI command "G0 X0 Y0 Z0 A0 B0 C0", _"Absolute/Relative"_ toggle button change mode of displayed positions and the third _"Use limits"_  allows to ignore limits when you need to move machine out from limit switches.

### Auto ###
![http://miniemc2.googlecode.com/files/miniemc2_1.png](http://miniemc2.googlecode.com/files/miniemc2_1.png)

It provides tools for NC program execution control and MDI operations. First horizontal panel contains next controls (from left to right):

1. **Program Stop** button. Click there stops current program or MDI command execution. Current NC files will be reload and Current Line Pointer will be set to first line of NC code.

2. **Current line number** input field. During program execution it shows actual program line. When program is stopped user can enter line number to start program from. _Note: before program start make sure does it point to correct line number_

3. **Start/Pause/Resume** button. This is multifunction button. If program is stopped (i.e. interpreter in the Idle state), then click on this button starts program execution. At this time button's will be switched to "Pause" mode. Clicking on "Pause" will suspend the program execution until user clicks on this button again (or click **Program Stop** button).

4. **Step** button. Click on this button starts execution of the current line of G-code. If program was started, then it suspends program execution at the end of current line.

5. **NC Program display** button switches **Data panel** to _Program view_ mode.

6. **File display** button switches **Data panel** to _File view_ mode ( see below).

Second (main) panel has name **Data panel**. It is used to display NC program (_Program view_) or file selection tree (_File view_).
_Program view_ displays current NC program's context (16 lines). Current line is highlighted. User's click on one of displayed lines will change value of **Current line number** input field.
_File view_ displays files and folders tree on mini2440 at _/media_ folder. User can select the program to be executed.

Last panel is **MDI** execution control. User can enter single line of valid G-code and press the button next to edit line to execute entered command.

### Misc ###
![http://miniemc2.googlecode.com/files/miniemc2_4.png](http://miniemc2.googlecode.com/files/miniemc2_4.png)

This panel allows:
  * Changing of IP address. To do that enter new IP address and click **Apply** button. Page will be reloaded from new address.
  * Reboot the system by clicking on **Reboot** button
  * Restart EMC2 core by clicking on **Reload** button. It's useful when user has changed EMC2's configuration files and would like to apply changes without reboot.




# Connections #
miniEMC2 has 19 digital inputs and 19 outputs connected to CPU's GPIO. Below connections table according to mini2440 schematics. _**Please note: you have not apply signals from mini2440 to steppper motor driver, or from switches and buttons directly to inputs of mini2440 - this may destroy your CPU. You need to use buffering circuit and may be logical level shifiting sircuit! CPU is not 5V tolerant!**_
| **Out #** | **Conn. #** | **Pin** |  | **In #** | **Conn. #** | **Pin** |
|:----------|:------------|:--------|:-|:---------|:------------|:--------|
| 0         | 4           | 9       |  | 0        | 4           | 21      |
| 1         | 4           | 10      |  | 1        | 4           | 22      |
| 2         | 4           | 11      |  | 2        | 4           | 23      |
| 3         | 4           | 12      |  | 3        | 4           | 24      |
| 4         | 4           | 13      |  | 4        | 4           | 25      |
| 5         | 4           | 14      |  | 5        | 4           | 26      |
| 6         | 4           | 15      |  | 6        | 4           | 27      |
| 7         | 4           | 16      |  | 7        | 4           | 28      |
| 8         | 4           | 17      |  | 8        | 20          | 1       |
| 9         | 4           | 18      |  | 9        | 20          | 2       |
| 10        | 4           | 19      |  | 10       | 20          | 3       |
| 11        | 4           | 20      |  | 11       | 20          | 4       |
| 12        | 20          | 11      |  | 12       | 20          | 5       |
| 13        | 20          | 12      |  | 13       | 20          | 6       |
| 14        | 20          | 13      |  | 14       | 20          | 7       |
| 15        | 20          | 14      |  | 15       | 20          | 8       |
| 16        | 20          | 15      |  | 16       | 20          | 9       |
| 17        | 20          | 16      |  | 17       | 20          | 10      |
| 18        | 4           | 31      |  | 18       | 4           | 32      |

Software package comes with the single configuration:
  * 4 linear axes with _mm_ unit
  * maximum axis speed is 720 mm/min
  * Step ratio: 3200 steps/revolution
  * Inputs for 4 home switches and E-Stop button
  * Outputs for _Motor Enable_ and _Spindle ON/OFF_
  * 1 PWM output connected to Spindle RPM

Below Input and Output pins table:
| Name | Number |Type |
|:-----|:-------|:----|
| Step X | 1      | Out |
| Dir X | 2      | Out |
| Step Y | 3      | Out |
| Dir Y | 4      | Out |
| Step Z | 5      | Out |
| Dir Z | 6      | Out |
| Step A | 7      | Out |
| Dir A | 8      | Out |
| Enable | 11     | Out |
| Spindle PWM | 16     | Out |
| Spindle | 0      | Out |
| Home X | 0      | In  |
| Home Y | 1      | In  |
| Home Z | 2      | In  |
| Home A | 3      | In  |
| E-Stop | 18     | In  |

# Configuring #
Configuration should be changed by hand, there is no any dedicated tools. Configuration files are stored at _/home/emc2/configs/_ folder. There are two files to be edited: _miniemc.ini_ and _minimec.hal_. First one was desribed in details in _EMC2 Integrator Manual_, so let's look at the HAL file:
```

loadrt trivkins
# motion controller, get name and thread periods from ini file
loadrt [EMCMOT]EMCMOT base_period_nsec=[EMCMOT]BASE_PERIOD servo_period_nsec=[EMCMOT]SERVO_PERIOD traj_period_nsec=[EMCMOT]TRAJ_PERIOD key=[EMCMOT]SHMEM_KEY num_joints=[TRAJ]AXES
loadrt miniemcdrv axes_conf="X Y Z A" fifo_deep=32 step_pins=1,3,5,7 dir_pins=2,4,6,8 dir_polarity=1,1,1,1 step_per_unit=320000,320000,320000,320000 io_update_period=1 max_pwm_value=30000 pwm_pin_num=16,17
# add motion controller functions to servo thread
addf motion-command-handler servo-thread
addf motion-controller servo-thread
# add miniemcdrv thread
addf update-miniemcdrv servo-thread
# Motion sync sygnals
net TrajWiat motion.traj-wait-ready  <= miniemcdrv.traj-wait-out
# create HAL signals for position commands from motion module
# loop position commands back to motion module feedback
net A0pos axis.0.motor-pos-cmd => miniemcdrv.0.cmd-pos
net A0pos_fb miniemcdrv.0.fb-pos => axis.0.motor-pos-fb
net A0home miniemcdrv.0.pin-in => axis.0.home-sw-in
setp miniemcdrv.0.pin-in-inv True

net A1pos axis.1.motor-pos-cmd => miniemcdrv.1.cmd-pos
net A1pos_fb miniemcdrv.1.fb-pos => axis.1.motor-pos-fb
net A1home miniemcdrv.1.pin-in => axis.1.home-sw-in
setp miniemcdrv.1.pin-in-inv True

net A2pos axis.2.motor-pos-cmd => miniemcdrv.2.cmd-pos
net A2pos_fb miniemcdrv.2.fb-pos => axis.2.motor-pos-fb
net A2home miniemcdrv.2.pin-in => axis.2.home-sw-in
setp miniemcdrv.2.pin-in-inv True

net A3pos axis.3.motor-pos-cmd => miniemcdrv.3.cmd-pos
net A3pos_fb miniemcdrv.3.fb-pos => axis.3.motor-pos-fb
net A3home miniemcdrv.3.pin-in => axis.3.home-sw-in
setp miniemcdrv.3.pin-in-inv True

net Enabled axis.0.amp-enable-out =>  miniemcdrv.11.pin-out
setp miniemcdrv.11.pin-out-inv False

net Spindled motion.spindle-on =>  miniemcdrv.0.pin-out
setp miniemcdrv.0.pin-out-inv False

# EStop is used
net EStop iocontrol.0.emc-enable-in <= miniemcdrv.18.pin-in
setp miniemcdrv.18.pin-in-inv False
# Connect spindle PWM output
net SSpindlePWM motion.spindle-speed-out => miniemcdrv.0.pwm-in
# create DI/DO signals
net Input4 miniemcdrv.4.pin-in => motion.digital-in-04
net Input5 miniemcdrv.5.pin-in => motion.digital-in-05
net Input6 miniemcdrv.6.pin-in => motion.digital-in-06
net Input7 miniemcdrv.7.pin-in => motion.digital-in-07
net Input8 miniemcdrv.8.pin-in => motion.digital-in-08
net Input9 miniemcdrv.9.pin-in => motion.digital-in-09
net Input10 miniemcdrv.10.pin-in => motion.digital-in-10
net Input11 miniemcdrv.11.pin-in => motion.digital-in-11
net Input12 miniemcdrv.12.pin-in => motion.digital-in-12
net Input13 miniemcdrv.13.pin-in => motion.digital-in-13
net Input14 miniemcdrv.14.pin-in => motion.digital-in-14
net Input15 miniemcdrv.15.pin-in => motion.digital-in-15
net Input16 miniemcdrv.16.pin-in => motion.digital-in-16
net Input17 miniemcdrv.17.pin-in => motion.digital-in-17
net Output9 motion.digital-out-09 => miniemcdrv.9.pin-out
net Output10 motion.digital-out-10 => miniemcdrv.10.pin-out
net Output12 motion.digital-out-12 => miniemcdrv.12.pin-out
net Output18 motion.digital-out-18 => miniemcdrv.18.pin-out
# create signals for tool loading loopback
net tool-prep-loop iocontrol.0.tool-prepare iocontrol.0.tool-prepared
net tool-change-loop iocontrol.0.tool-change iocontrol.0.tool-changed
```
First unusual line is
```

loadrt miniemcdrv axes_conf="X Y Z A" fifo_deep=32 step_pins=1,3,5,7 dir_pins=2,4,6,8 dir_polarity=1,1,1,1 step_per_unit=320000,320000,320000,320000 io_update_period=1 max_pwm_value=30000 pwm_pin_num=16,17
```
**miniemcdrv** it's only HW drive that can be used in miniEMC2, so I have to describe it enough.
## Description ##
**miniemcdrv** take care about following functions:
  * Receives commanded position from the _motion controller_ and caclulate desired values for Step's frequency synthesizers, add it to dedicated FIFO;
  * Implements PI regulators of position loops;
  * Provides HAL for digital inputs and outputs;
  * Transfer Digital Ouputs and PWM values to FIQ handler;
  * Poll Digital Inputs;
## Parameters ##
`axes_conf` - describes axes configuration. It is a string of capital letters **X**, **Y**, **Z**, **A**, **B**, **C** delimited by `space` symbol. Imagine, our axis was numbered from 1 to 6 and we have next configuration string "X Y Z X A Y". It means  we assigned axis1=**X**, axis2=**Y**, axis3=**Z**, axis4=**X**, axis5=**A** and axis6 =**Y**. We have two axes with name **X**: 1 and 4, that is they will moves synchronously, like another pair of axes: 2 and 6 with name **Y**. Maximum 6 axis allowed.

`fifo_deep` - optional parameters which sets frequency synthesizerss' FIFO deep. For usual users it means delay in mS between comnanded and actual position. Less value - less delay. But at the same time you have not set its value lower than 8, because it affects also to system stability. If you woulld like to make tests, then connect serial console and look for any "FIFO underrun!!!" messages. If they appeared, then you have set too small `fifo_deep` value. _This option only for test purpose and will be removed in the future_.

`step_pins` = list of comma-separated values of Digital Output pins to be used as STEP signals. Accepted values: 0 .. 18, or -1 if no pins assigned. Number of pins should match to number of axes in `axes_conf` parameter.

`dir_pins` = list of comma-separated values of Digital Output pins to be used as DIRECTION signals. Rules are the same as for `step_pins`.

`io_update_period` = only for test, set it to 1.

`step_per_unit` = list of comme-separated positive integer values. Each value represent number of STEP pulses per axis unit multiplyed by 100. In the example above value 320000 means 320000/100=3200 pulses/unit. Up to 6 values allowed.

`max_pwm_value` = fullscale value of PWM, single value for both channel. If you use it for spindle speed control, then set it to MAX Spindle RPM.


`pwm_pin_num` = number of Output Pins to be used as PWM output. Two values allowed, because we have 2 PWM channels.

## Functions ##
`miniemcdrv.update-miniemcdrv` should be called at servo update rate.

## Pins ##
`miniemcdrv.N.pin-out` - input. Represent Digital Output (DO), N=0..18;

`miniemcdrv.N.pin-out-inv` - input, allows to invert corresponding DO, N=0..18;

`miniemcdrv.N.pin-in` - output. Represent Digital Input (DI), N=0..18;

`miniemcdrv.N.pin-in-inv` - input, allows to invert corresponding DI, N=0..18;

Next important line

```

# Motion sync sygnals
net TrajWiat motion.traj-wait-ready  <= miniemcdrv.traj-wait-out
```

This line should be present always in any configuration, it allows to control FIQ'
s FIFO overflow.


# To be continued... #

**_Request to all who is reading this manual: I would like to know your opinion about this project: What do you want me to desribe in this manual? Is is clear how to use WEB interface? Is it usable? So any feedback are welсome! Email me to kayserg at gmail.com_**
















