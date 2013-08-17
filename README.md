calyx
=====
the beautiful thing to nourish your hummingbird :)

![alt text](http://moc.sirtetris.com/calyx0.jpg "calyx")

features
--------
* view your hummingbird.me currently-watching list
* change number of watched episodes
* integration of xdcc bot packlists
* display filtered packlist or raw packlist lines

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
* **l** -> increase watched episodes count by one
* **h** -> decrease watched episodes count by one
* **s** -> if available, show packlist info
* **c** -> change show packlist info display mode
* **r** -> refresh bot lists
* **q** -> quit

todo
----
* MAKEFILE
* cyclic refresh (botwatch)
* list scrolling
* indication of blocking procedures (api requests)
* display of list other than currenly watching

note
----
This is the first piece of software I write in C, so the code will most likely suck, A LOT.
