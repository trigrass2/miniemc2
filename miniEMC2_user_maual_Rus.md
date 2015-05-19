# Введение #
MiniEMC2 это адаптированная версия широко известной программы управления станком ЧПУ EMC2. Это автономный контроллер ЧПУ, который позволяет вам управлять станком без использвани PC через TCP/IP сеть.  MiniEMC2 формирует сигналы управления ШАГ/НАПРАВЛЕНИЕ для управления шаговыми двигателями. Основные характеристики:
  * До 6 осей ( должно быть уточнено по результатом тестов )
  * Частота шагов до 40 кГц
  * Длительность и полярность импульса ШАГ фиксирована: 10 uS, положительная
  * 19 цифровых входов и 19 выходов. Каждый из них может быть проинвертирован.
  * Каждый выход может быть использован как сигнал ШАГ/НАПРАВЛЕНИЕ/ШИМ или Цифровой Выход общего назначения ( может управляться из NC программы).
  * Каждый вход может быть использован как вход для концевых выключателей, кпопки Стоп или как Цифровой Вход общего назначения.
  * 2 канала ШИМ с фиксированной частотой и 100 возможными значениями длительности.
  * Выполнения программы возможно как с SD карты, так и с USB Flash диска.
  * FTP доступ для загруски NC программ.
  * Управление по сети через TkEMC
  * Оригинальный WEB интерфейс для управления по сети
  * Возможность подключения внешней панели управления (кнопок через _halui_ )

# Отличия от оригинального EMC2 #
В качестве базы была выбрана версия 2.2.0, позже были добавлены изменеия из версии 2.2.8, т.е. это практически версия 2.2.8. Были внесены некоторые изменения в архитектуру EMC2 из-за особенностей процессора S3C2440. Во-первых вместо RTAI используется Xenomai. В данной версии отсутствует отдельная RTAPI абстракция для Xenomai, вместо этого были внесены изменения в SIM RTAPI таким образом, что вместо _libpth_ используются вызовы Xenomai native API.
Стандартные аппаратные драйверы не используются из-за их низкой производительности на данной платформе. Вместо этого используется решение основанное на обработке "быстрых" прерываний (FIQ) от таймера с периодом 10 мкс. Это решение было заимствовано в проекте  OpenMoko. Стоит отметить одну особенность данного решения. _Для повышения стабильности работы синтезаторов частоты был добавлена так называемая "очередь координат", практически это означает некоторую задержку между заданной и актуальной позицией. Эта задержка может быт вычислена как N\*1mS, где N - размер "очереди координат". По умолчанию N=32, но может быть уменьшено пользователем до 8. Эта задержка добавляет некоторые ограничения на область использования данного проекта_.
Взаимодействие между _FIQ_ и _motion controller_ обеспечивается специальным драйвером HAL по имени **modminiemc**. Он реализует практически те-же функции, что и компоненты _parport_ и _stepgen_ обычного EMC2. Все остальные драйверы аппаратуры _не поддерживаются_, однако такие HAL компоненты, как логические примитивы, регуляторы (т.е. аппаратно независимы компоненты) доступны пользователю.
miniEMC не использует штатные дисплеи от mini2440 из-за низкой производительности GUI совместно с EMC2. Пользователь может выбрать между удаленным управлением через TkEMC или специальной WEB-панелью управления. Количество одновременно работающих осей остается открытым вопросом. Синтезатор частоты поддерживает до 6 осей, но глубокое тестирование в этом режиме не проводилось. 4 оси работаю нормально.
И последнее: _стабильно работает только тривиальная кинематика_, нетривиальная так-же присутствует, но ей не хватает вычислительной мощности данного CPU.

# Быстрый старт #
Архив с программами содержит три части: начальный загрузчик u-boot, образ ядра Linux и образ файловой системы ( а так-же в виде tar архива). Последний релиз можно скачать в разделе Downloads этого сайта. Кроме того вам понадобится mini2440 без дисплея, соединенный с вашим ПК по RS-232 ( COM-порт ). Плата должна быть подключена к сети Ethernet. Также желательно установить SD карту в слот mini2440. Со стороны ПК нужно установить две программы: TFTP-сервер ( например TFTP32 для Windows ) и программу для обмена по последовательному порту, например PuTTY. Распакуйте все файлы из архива EMC2 в корневую папку TFTP-сервера.

Сначала нужно устовить u-boot. Это процедура неплохо описана в Интернете, например [здесь](http://www.google.com/url?url=http://wiki.linuxmce.org/index.php/Mini2440%23Flashing_uboot&rct=j&q=mini2440+u-boot+setup&usg=AFQjCNGKmz5THZr0yw7V5xiD08asalhM6Q&sa=X&ei=XOK2Tp2EIuP34QTalqnJBQ&ved=0CCYQygQwAQ&cad=rja).
Образ загрузчика, предоставляемый в нашем архиве, практически ни чем не отличается от оригинального с сайта по ссылке за исключением нескольких переменных окружения. Если вы уже имеете установленный u-boot, то можно просто добавить эти переменный с консоли:
```

set update_kernel 'tftpboot 0x32000000 uImage; nand erase 0x60000 0x500000; nand write.e 0x32000000 0x60000 0x500000'
set update_fs 'tftpboot 0x32000000 rootfs.jffs2; nand erase 0x560000 0x2000000; nand write.e 0x32000000 0x560000 ${filesize}'
set bootargs 'bootargs=console=ttySAC0,115200n8 root=/dev/mtdblock3 rootfstype=jffs2'
set bootcmd 'nand read.e 0x32000000 0x60000 0x500000; bootm 0x32000000'
set update 'run update_kernel;run update_fs;run bootcmd'
saveenv
```
После этого установите IP адрес вашей платы и адрес TFTP-сервера:
```

set ipaddr 192.168.1.80
set serverip 192.168.1.8
run update
```
Замените _192.168.1.8_  правильным адресом вашего TFTP сервера. Последняя строка начинает процедуру загрузки образов ядра и файловой системы и запись их в NAND flash, после этого miniEMC2 стартует автоматически. Первая загрузка потребует минуту или две, после чего WEB-сервер будет доступен по адресу _192.168.1.80_. Кроме того устройство будет доступно по FTP и SSH ( пользователь **root**, пароль **root** ). SD карта и USB Flash должны монтироваться и размонтироаться автоматически. Их содержимое находится в папках _/media/mmc_ and _/media/usb_. _Внимание: поддерживается монтирование только одного ( последнего ) раздела_. Тип файловой системы может быть FAT16/FAT32/EXT3.

Все образы были проверены на mini2440 с объемом NAND в 256Мб и 1Гб.

# WEB interface #
miniEMC2 использует полностью статическую WEB-страницу, которая загружается при первом соединении с WEB-сервером. После этого информация на дисплее обновляется динамически используя AJAX-запросы. Такое решение позволило получить минимальную нагрузку на mini2440, ускорить отображение страницы и дает пользователю возможность самостоятельно модифицировать вид WEB-страницы. Для этого нужно модифицировать всего три текстовый файла: index.html, css/webemc.css и js/miniemc2.js.

WEB-сервер доступен по адресу htpp://192.168.1.80 через минуту после загрузки mini2440. Вы увидите следующую страницу:

![http://miniemc2.googlecode.com/files/miniemc2_3.png](http://miniemc2.googlecode.com/files/miniemc2_3.png)

Она разделена на 4 части:

## Панель оперативной информации и управления ##
Это левая панель страницы. Как вы видите здесь отображаются _Заданная позиция осей_, слайдер для изменения скорости подачи в процентах от заданной и кнопка _Аварийного останова_. Эта панель отображается всегда не зависимо от режима или выбранной вкладки управления. В данной версии положение осей всегда отображается в **мм** независимо от заданного в программе. _Внимание: не пытайтесь перетаскивать слайдер подачи, просто кликните сверху или снизу него_.

Кнопка _Аварийный останов_ имеет три состояния. _Нормальное_ когда машина находится в неактивном состоянии, _Включено_ ( отображается красный крест внутри кнопки) когда машина готова к работе и все привода осей включены и/или перемещаются. Нажатие на кнопку в этот момент приведет к немедленному ( аварийному ) останову машины. И третье состояние - _Авария_, когда кнопка отображается красным цветом. Это обозначает поступает внешний сигнал _Аварийный останов_. При этом машина переводится в выключенное состояние и находится в нем пока этот сигнал присутствует.

## Панель управления машиной ##
Это правая часть страницы. Здесь могут отображаться 4 вида панели в зависимости от выбора пользователя в _Панели меню_ ( верхняя панель ).

### Ручное управление ###
Это панель по умолчанию при старте WEB-сессии (см. рисунок выше)
Пользователь может перемещать оси используя специальные кнопки управления.  Для осей **X** и **Y** есть специальные кнопки в виде джойстика. Внутри него располагается тумблер включения шпинделя.
Все остальные оси могут перемещаться при помощи круглых кнопок, при этом перемещаться будет та ось, имя которой ранее было выбрано в элементе управления _Select axis_.  Другой элемент управления  с именем _Jogging mode_ позволяет выбрать один из 9 типов перемещения: _cont._ - непрерывное перемещение пока нажата соответствующая кнопка и 8 возможных перемещений на фиксированную длину 500, 100, 10, 1, 0.1, 0.01, 0.001 мм.

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
