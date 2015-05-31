This is a simple reimplementation of Google's [SparseHash](https://code.google.com/p/sparsehash/)
library intended as both a learning and teaching excercise.

## How do I use it?

Either copy `./include/simple_sparsehash.h` and `./src/simple_sparsehash.c` into
your project and start using them, or build this project and link `-lsimple-sparsehash`.

## Differences between the official version

* Doesn't support many of the things that the official version does, like
  iterators, swapping, deletion, etc.
* There are no 'default values' of sparse arrays. You access something that
  isn't real? You get `NULL`.

## Eventual TODO

* Store actual items in the arrays, not pointers to items.
* Resize the table down when it reaches an inverse occupancy or something.
* Store object size in the dictionary, so that we can make assumptions about
  array size. Right now it accepts any value, and is slightly slower due to not
  having any locality of reference, and having to jump to an extra location in
  memory. Maybe two different versions?
