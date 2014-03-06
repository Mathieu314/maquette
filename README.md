Home Control
========
This program uses a Raspberry Pi.

How to install and run this program
========
First you have to install the WiringPi library on Raspian.
Please se at https://github.com/WiringPi/WiringPi<br/>
Then compile by typing following in a terminal emulator :
<code>gcc home-control.c -o home-control -lpthread -lwiringPi</code>

Then run what you just compiled with :

<code>./home-control</code>

Then you should see a prompt like this :

<code>Salut !<br/>>: </code><br/>
Yes, texts and commands are in french ! (Because I'm French)

For more informations about the available commands, try <code>aide</code> while running the programm, or se the "aide.txt" file.

Legal informations
=======
This software is distributed under the GNU General Public License

About the author
======
Mathieu Moneyron<br/>
Please contact me at <mathieu.moneyron@laposte.net>
