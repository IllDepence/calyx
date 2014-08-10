calyx
=====
the beautiful thing to nourish your hummingbird :)

![alt text](http://moc.sirtetris.com/calyx.gif "calyx")

features
--------
* view your hummingbird.me currently-watching list
* change number of watched episodes
* integration of xdcc bot packlists
* display filtered packlist or raw packlist lines

setup
-----
* compile: gcc -o calyx calyx.c -lcurl -ljson-c -lncurses
* create file calyx.conf (see example)
* adjust file bot\_watch.json
* launch

usage
-----
* **k** -> navigate up
* **j** -> navigate down
* **l** -> increase watched episodes count by one
* **h** -> decrease watched episodes count by one
* **s** -> if available, show packlist info
* **c** -> change packlist info display mode
* **r** -> refresh bot lists
* **q** -> quit

todo
----
* MAKEFILE
* cyclic refresh (botwatch)
* list scrolling
* indication of blocking procedures (api requests)
* display of lists other than currenly watching

notes
-----
* This is the first piece of software I wrote in C, so the code will most likely suck, A LOT.
* As soon as AniList gets it's API I'll create a similar ncurses client — this time in Python.
