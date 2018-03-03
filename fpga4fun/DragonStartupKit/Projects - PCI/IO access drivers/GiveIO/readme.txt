How to install the GiveIO Driver for Window 2000/NT
====================================================
Compiled by RADiCAL/THE SAiNTz

1. WHY WE HAVE TO USE IT

Well, NT/2000 is a great operating system which has lots of stuff protecting
programs from directly accessing the systems hardware. But sadly most cheat
cartridges only have Windows 95/98 or DOS software to up/download memory
cards, codes or to flash the BIOS. CAETLA is a good example. You just have
DOS tools and in order to use them you have to give your DOS Prompt full
hardware access.

2. INSTALLATION

To install this driver on Windows NT/2000 you will have to have Administator
rights on your machine !

Step 1: Install the driver using LOADDRV.EXE

Unzip the package in the directory you want to have you driver be installed
(c:\Program Files\GiveIO). Afterwards run the program LOADDRV.EXE also enclosed
in this package. Enter the FULL PATH to the GIVEIO.SYS driver e.g. 
C:\Program Files\GiveIO\GIVEIO.SYS and click the install button. Do not press
return since this will end the program. 

The GiveIO-Service should have been installed. Now use the START button to
start the service (you may also use the Control-Panel). If the text
"Operation Successful" occurs everything worked fine.

In case you get an error message click REMOVE, check the paths, click INSTALL
and START.

Step 2: Activate the driver using ACTIVATE.CMD

The activation of the driver is quite strange but I put a batch-file in the
package called GiveIO.CMD, which starts a new prompt on which you can 
access all hardware ports.

Notes:

 * NT/2000 somewhat messes up the LPT Port adresses, for example, my x-Ploder
   is connected to LPT1 but I have to use the -P11 switch on all Caetla Tools
   (which normally means LPT2)

3. CREDITS FOR THE SOFTWARES

None of these programs have been written by me. I take no credit for them.

GIVEIO.SYS by Dale Roberts, 
  available via http://www.dc.ee/Files/Programm.Windows/directio.zip
LOADDRV.EXE by Paula Tomlinson, 
  available via ftp://ftp.mfi.com/pub/windev/1995/may95.zip

5. GREETINGS

Greetz fly out to Daniel, HiLyte, Ali, Dirk, all former members of the SAiNTz
