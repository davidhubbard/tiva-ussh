libti2cit
=========

This library was born because the Connected Launchpad i2c hardware state machine is complicated.
The i2c state machine of the MSP430 and AVR lines are so simple it hardly makes sense that it should
be this hard to use i2c on the Connected Launchpad.

The goal of this library is to make your app a lot less painful to write. Huge success.

**Table of Contents**
- [Installation](#installation)
- [Writing Your Own i2c Application](#writing-your-own-i2c-application)
- [libti2cit HOWTO](#libti2cit-howto)
- [Understanding i2c](#understanding-i2c)
- [License](#license)

Installation
------------

You need to extract one file from inside the Zip file
[SW-EK-TM4C1294XL-2.1.0.12573.exe](http://www.ti.com/tool/sw-ek-tm4c1294xl) (Yes, it is a
zip file and any unzip program on any modern OS will extract the contents. It *does* spew its
files into the current dir though. Fair warning.)

`project.ld` <-- Find this file.

It is used by `ld` to assemble `example-main.bin` when you type `make`. It is the formal definition
of how to assemble the flash image for a Connected Launchpad, so get it from the zip file available
on TI.com. I found it under `examples/project/project.ld`. Copy `project.ld` into the current
directory where this README lives.

Then run `make`. You should see something like:

```
$ make
  CC    example-main.c
  CC    startup_gcc.c
  LD    example-main.elf 
$
```

Finally you can run `make lm4flash` or just run `lm4flash` yourself. This will program the
flash image to your device. (Advanced users may choose to program the device with another
tool than lm4flash.)

Writing Your Own i2c Application
--------------------------------

The easiest way to write an i2c application from this code is to use `example-main.c` as a template
and iterate until it meets your requirements.

* Embedded development protip: compile and flash a working build to your device, early and often.

libti2cit is designed to facilitate that.

In `Makefile` you can replace all occurrences of "example-main" with your own app name. Then
rename the `example-main.c` file to that name.

Libti2cit is licensed with a liberal LGPL license to make this code as widely available as is
possible. If you need to obtain a different license, please create an issue on the repository
at github.com and include your contact information.

libti2cit HOWTO
---------------

So how do you use libti2cit in your application?

1. Do the [installation](#installation) step to copy project.ld into the source directory.

2. Use the Tivaware DriverLib to initialize the hardware. It really does just fine at that.

  a. Find the base address for the right i2c port. For example, I2C0_BASE, I2C1_BASE, I2C2_BASE, etc.

  b. Reset the i2c hardware, wait for it to complete its reset, configure the pin functions, etc.
     (This is all part of the Tivaware DriverLib initialization.)

3. You probably want to [understand i2c](#understanding-i2c) to correctly design your application.
   Most of this planning only affects the init code you write using the Tiva DriverLib.
   Here is a decision checklist:

  a. Master or Slave mode.

  b. Hardware acceleration:
    * Polling, no FIFO: the simplest way to write and debug code.
    * Polling, FIFO Burst: slightly more efficient. (Note: I decided not to implement this mode
      because power use and system throughput are not that much better than the no FIFO mode.)
    * Interrupts, no FIFO: use the interrupt controller to free up the Connected Launchpad during
      an i2c transfer.
    * Interrupts, FIFO Burst: add the FIFO (First-in First-out queue) so only minimal interrupts
      are needed.
    * Interrupts, FIFO Burst, uDMA: use uDMA instead of the interrupt handling code, so even
      High-Speed mode transfers will not use much CPU power.
  
  c. Which i2c pins on the Connected Launchpad are connected to your i2c devices.
  
  d. Install an interrupt handler and enable interrupts for all the i2c controllers you are using.
  
  e. Gather the i2c device addresses and the documentation or driver source code for your devices.

4. If you initialize the launchpad i2c controller in **Master** mode:

  a. The address you send must be left-shifted by 1. The LSB or 1's bit is used to
    signal a read or write, and is not available for addresses. In other words, i2c addresses can
    be viewed as 0-127 (before the left-shift by 1) which become even-numbered addresses 0-254
    (after the left-shift by 1). The odd addresses tell the slave that a read is requested.

  b. Only call `libti2cit_m_...` functions. Do not call `libti2cit_s_...` functions in Master mode.

  c. You have to ask the slave for data to read by first calling `libti2cit_m_sync_send()`. This might
    seem confusing, but it's how i2c works:
    
    `libti2cit_m_sync_send((address << 1) | 1)`
    
    The LSB is set to 1 during the send. Then call `libti2cit_m_recv()` to receive bytes from the slave.
    The Connected Launchpad i2c hardware can get stuck and never flip the expected bits if you call
    functions in the wrong sequence, so always call a `send()` before a `recv()`.

  d. You must give up and restart from `send()` if an error occurs. Check return values.
  
  e. `libti2cit_m_sync_send()` is the polling (no FIFO) version. Replace `_sync_` with `_isrdma_` /
    `_isr_` / `_isr_nofifo_` for the other modes. The interrupt-based functions do not have return
    values, but will indicate an error in the status argument passed to your callback in `user_cb`.

libti2cit HOWTO for Slaves
--------------------------

1. These are the Tivaware initialization steps:
  * `SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C`*n*`)` where *n* is which i2c controller you want.
  * `SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIO`*a*`)` where *a* is which GPIO port has the i2c pins.
  * `while (!SysCtlPeripheralReady(SYSCTL_PERIPH_I2C`*n*`));` wait for the controllers to come up.
  * `while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIO`*a*`));` wait for the controllers to come up.
  * `GPIOPinConfigure(GPIO_P`*a*`_I2C`*n*`_SDA);`
  * `GPIOPinConfigure(GPIO_P`*a*`_I2C`*n*`_SCL);`
  * `GPIOPinTypeI2C(GPIO_PORT`*a*`_BASE, GPIO_PIN_`*b*`);`
  * `GPIOPinTypeI2CSCL(GPIO_PORT`*a*`_BASE, GPIO_PIN_`*b*`);`
  * `I2CSlaveInit(I2C`*n*`, `slave address`);` to set the slave address.
  * `libti2cit_s_int_clear();` to empty the interrupt controller of stale i2c interrupts.
  * `IntMasterEnable();`
  * `IntEnable(INT_I2C`*n*`);`
  * `I2CSlaveIntEnableEx(`*base*`, I2C_SIMR_DATAIM);` Only `_DATAIM` is needed for slave mode.
  * In your interrupt handler: `status = libti2cit_s_isr_isr();` and then check `status` for
    `LIBTI2CIT_ISR_...` constants and `I2C_SCSR_...` constants ORed together.
  
2. Only call `libti2cit_s_...` functions. Do not call `libti2cit_m_...` functions in Slave mode.
  Of course, the example code sets up a single i2c controller in master+slave mode (loopback mode),
  and proceeds to happily mix both kinds of function calls... but they are in separate code paths,
  which helps, I guess.

3. There is no way to control how little or how much data the master will transfer. The slave may
  receive an incomplete packet or may receive extra bytes; must gracefully give up on a packet if
  the master switches back to transmittion; and must gracefully handle sending more bytes when
  the master reads beyond the end of a packet.

4. Gracefully handling master demands makes your i2c slave code more flexible, reliable, and
  useful. The cake is a lie.

Understanding i2c
-----------------

[i2c is a brilliantly simple protocol](http://en.wikipedia.org/wiki/I2C) that only uses 2 wires.

The 2 wires are:
- SDA ("Serial Data")
- SCL ("Serial Clock")

Both wires must have one pull-up resistor each. It turns out the Connected Launchpad can even generate an
"internal pullup" if you want. Since that is part of the Tivaware DriverLib initialization step, it is
not described here.

After all your devices are connected to SDA and SCL, you must decide which devices are to be a "master"
and which are to be a "slave." Don't worry: electronic devices don't have feelings and are perfectly happy
to be enslaved on an i2c bus. **Master devices are responsible for starting (who, what, when, where) and
ending every transaction that goes across the bus.** Also, most i2c devices you might try to hook up to
your Connected Launchpad are slaves and cannot be a master anyway. However, the Connected Launchpad can
be set up as a slave too, if that better fits your design.

An i2c slave can never spontaneously send data to its master. This may mean the master has to check
back with the slave constantly, which becomes a lot of work for the master. But it can keep your code
clean because the master controls all the timing.

An i2c master does not have an address. Usually one i2c bus has only one i2c master, and I'm going to
leave out the explanations for a bus with multiple masters. I'll only say, for illustration purposes,
that if a bus has 2 masters, they can listen to each other's conversations but they cannot directly
talk to each other.

**The simple case: Launchpad in Master mode**

Since almost everyone using libti2cit wants to run the Launchpad as the i2c master, in this section
we will assume the Launchpad has been initialized in master mode. See the Tivaware example code and
Processor Data Sheet from TI for initialization instructions.
libti2cit works both for master and slave mode.

In master mode, you call `libti2cit_m_sync_send()`. The `m` is for master, and
`libti2cit_m_sync_send()` is what wakes up all the slaves and takes command of the bus. This
is called an i2c START.

First the master sends i2c START, then it sends an address, then zero or more bytes of data, and
last of all (if the master wants to receive bytes), the slave at the address can start talking and
sends data to the master.

The master can "let go of the i2c bus" by sending an i2c STOP at any time. A slave can only talk when
the master sends the address of that slave device, and must continue sending bytes until the master
sends the i2c STOP.

**Receiving as Master**

Say you are the master. Say you want to read something. You can't just `libti2cit_m_sync_recv()`,
but must call `libti2cit_m_sync_send()` first. The address must be ORed with 1 to ask to receive
data from the slave. Finally you can call `libti2cit_m_sync_recv()`. Also look at the slave device's
datasheet or library source code and examples. Each device will want different bytes during the
`send()` and `recv()` stages of an i2c bus transaction.

The `libti2cit_m_sync_recv()` will happily read forever, even bogus data the slave did not send.
The i2c bus does not include a signal **from the slave** saying that `libti2cit_m_sync_recv()`
succeeded or failed. The received data must be checked for validity, and if invalid, thrown out.
(Typically a value of 255 or 0xff indicates failure because that is what the i2c bus looks like
when no one is talking at all.)

The Tivaware example code and Processor Data Sheet from TI include important details you must
understand to correctly initialize the i2c controllers you want to use.

**Scanning the Bus as Master**

One interesting thing the i2c master can do is scan the bus. If the master sends an i2c START,
an address, and immediately an i2c STOP, the slave at each address is required to respond if
present. The slave sends no data, just an "I am here," so this can be used to scan the bus.
Some i2c slave devices (specifically SMBus devices) power on/off each time they are scanned
in this manner.

Here is an example of scanning the bus:

```
int main()
  init_i2c();

  uint32_t addr;
  for (addr = 0; addr < 128; addr++) {
    if (libti2cit_m_sync_send(base, addr << 1, 0 /*len*/, 0 /*buf ptr*/)) continue;
    UARTprintf("found %u\r\n", addr);
  }
  return 0;
}
```

**Sending and receiving as Slave**

Once you have [initialized the Tiva hardware to Slave mode](#libti2cit-howto-for-slaves),
you can only hope your master wants to talk to you.

Once the interrupt handler fires and you have status bits, those bits determine whether the
master just scanned you (`I2C_SCSR_QCMDST`), wants you to transmit data (`I2C_SCSR_TREQ`),
or wants you to receive data (`I2C_SCSR_RREQ`).

If the data doesn't make sense, ignore it. But do not ignore `I2C_SCSR_RREQ`: when this bit is
set the i2c controller is holding the clock line until you feed it a byte. The master may
freeze while waiting for your i2c controller will let go of the clock line.

If you get `I2C_SCSR_RREQ` and do not want to send data, send 0xFF to indicate you have nothing
to say.

License
-------

The tiva-ussh i2c library is licensed under the GNU Lesser General Public License, either version 3.0,
or any later version.


GNU LGPL v3
-----------

GNU LESSER GENERAL PUBLIC LICENSE
                       Version 3, 29 June 2007

 Copyright (C) 2007 Free Software Foundation, Inc. <http://fsf.org/>
 Everyone is permitted to copy and distribute verbatim copies
 of this license document, but changing it is not allowed.


  This version of the GNU Lesser General Public License incorporates
the terms and conditions of version 3 of the GNU General Public
License, supplemented by the additional permissions listed below.

  0. Additional Definitions.

  As used herein, "this License" refers to version 3 of the GNU Lesser
General Public License, and the "GNU GPL" refers to version 3 of the GNU
General Public License.

  "The Library" refers to a covered work governed by this License,
other than an Application or a Combined Work as defined below.

  An "Application" is any work that makes use of an interface provided
by the Library, but which is not otherwise based on the Library.
Defining a subclass of a class defined by the Library is deemed a mode
of using an interface provided by the Library.

  A "Combined Work" is a work produced by combining or linking an
Application with the Library.  The particular version of the Library
with which the Combined Work was made is also called the "Linked
Version".

  The "Minimal Corresponding Source" for a Combined Work means the
Corresponding Source for the Combined Work, excluding any source code
for portions of the Combined Work that, considered in isolation, are
based on the Application, and not on the Linked Version.

  The "Corresponding Application Code" for a Combined Work means the
object code and/or source code for the Application, including any data
and utility programs needed for reproducing the Combined Work from the
Application, but excluding the System Libraries of the Combined Work.

  1. Exception to Section 3 of the GNU GPL.

  You may convey a covered work under sections 3 and 4 of this License
without being bound by section 3 of the GNU GPL.

  2. Conveying Modified Versions.

  If you modify a copy of the Library, and, in your modifications, a
facility refers to a function or data to be supplied by an Application
that uses the facility (other than as an argument passed when the
facility is invoked), then you may convey a copy of the modified
version:

   a) under this License, provided that you make a good faith effort to
   ensure that, in the event an Application does not supply the
   function or data, the facility still operates, and performs
   whatever part of its purpose remains meaningful, or

   b) under the GNU GPL, with none of the additional permissions of
   this License applicable to that copy.

  3. Object Code Incorporating Material from Library Header Files.

  The object code form of an Application may incorporate material from
a header file that is part of the Library.  You may convey such object
code under terms of your choice, provided that, if the incorporated
material is not limited to numerical parameters, data structure
layouts and accessors, or small macros, inline functions and templates
(ten or fewer lines in length), you do both of the following:

   a) Give prominent notice with each copy of the object code that the
   Library is used in it and that the Library and its use are
   covered by this License.

   b) Accompany the object code with a copy of the GNU GPL and this license
   document.

  4. Combined Works.

  You may convey a Combined Work under terms of your choice that,
taken together, effectively do not restrict modification of the
portions of the Library contained in the Combined Work and reverse
engineering for debugging such modifications, if you also do each of
the following:

   a) Give prominent notice with each copy of the Combined Work that
   the Library is used in it and that the Library and its use are
   covered by this License.

   b) Accompany the Combined Work with a copy of the GNU GPL and this license
   document.

   c) For a Combined Work that displays copyright notices during
   execution, include the copyright notice for the Library among
   these notices, as well as a reference directing the user to the
   copies of the GNU GPL and this license document.

   d) Do one of the following:

       0) Convey the Minimal Corresponding Source under the terms of this
       License, and the Corresponding Application Code in a form
       suitable for, and under terms that permit, the user to
       recombine or relink the Application with a modified version of
       the Linked Version to produce a modified Combined Work, in the
       manner specified by section 6 of the GNU GPL for conveying
       Corresponding Source.

       1) Use a suitable shared library mechanism for linking with the
       Library.  A suitable mechanism is one that (a) uses at run time
       a copy of the Library already present on the user's computer
       system, and (b) will operate properly with a modified version
       of the Library that is interface-compatible with the Linked
       Version.

   e) Provide Installation Information, but only if you would otherwise
   be required to provide such information under section 6 of the
   GNU GPL, and only to the extent that such information is
   necessary to install and execute a modified version of the
   Combined Work produced by recombining or relinking the
   Application with a modified version of the Linked Version. (If
   you use option 4d0, the Installation Information must accompany
   the Minimal Corresponding Source and Corresponding Application
   Code. If you use option 4d1, you must provide the Installation
   Information in the manner specified by section 6 of the GNU GPL
   for conveying Corresponding Source.)

  5. Combined Libraries.

  You may place library facilities that are a work based on the
Library side by side in a single library together with other library
facilities that are not Applications and are not covered by this
License, and convey such a combined library under terms of your
choice, if you do both of the following:

   a) Accompany the combined library with a copy of the same work based
   on the Library, uncombined with any other library facilities,
   conveyed under the terms of this License.

   b) Give prominent notice with the combined library that part of it
   is a work based on the Library, and explaining where to find the
   accompanying uncombined form of the same work.

  6. Revised Versions of the GNU Lesser General Public License.

  The Free Software Foundation may publish revised and/or new versions
of the GNU Lesser General Public License from time to time. Such new
versions will be similar in spirit to the present version, but may
differ in detail to address new problems or concerns.

  Each version is given a distinguishing version number. If the
Library as you received it specifies that a certain numbered version
of the GNU Lesser General Public License "or any later version"
applies to it, you have the option of following the terms and
conditions either of that published version or of any later version
published by the Free Software Foundation. If the Library as you
received it does not specify a version number of the GNU Lesser
General Public License, you may choose any version of the GNU Lesser
General Public License ever published by the Free Software Foundation.

  If the Library as you received it specifies that a proxy can decide
whether future versions of the GNU Lesser General Public License shall
apply, that proxy's public statement of acceptance of any version is
permanent authorization for you to choose that version for the
Library.
