calyx
=====
the beautiful thing to nourish your hummingbird :)

setup
-----
* compile: gcc -o calyx calyx.c aux.o -lcurl -ljson-c -lncurses
* create file calyx.cfg (see example)
* adjust file bot\_watch.json
* launch

usage
-----
* **j** -> down
* **k** -> up
* **s** -> if available, show packlist info
* **l** -> increase watched episodes count by one
* **h** -> decrease watched episodes count by one
* **q** -> quit

status
------
basic functionality implemented

todo: 
* MAKEFILE
* cyclic refresh (botwatch)
* list scrolling
* indication of blocking procedures (api requests)

note
----
This is the first piece of software I write in C, so the code will most likely suck, A LOT.
