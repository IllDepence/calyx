calyx
=====
the beautiful thing to nourish your hummingbird :)

setup
-----
* compile: gcc -o calyx calyx.c aux.o -lcurl -ljson-c -lncurses
* create file calyx.cfg (see examples)
* launch

usage
-----
* **j** -> down
* **k** -> up
* **l** -> increase watched episodes count by one
* **h** -> decrease watched episodes count by one
* **q** -> quit

status
------
basic functionality implemented

todo: 
* list scrolling
* indication of blocking procedures (api requests)

note
----
This is the first piece of software I write in C, so the code will most likely suck, A LOT.
