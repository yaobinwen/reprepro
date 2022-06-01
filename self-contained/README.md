# Self-contained Header Files

## What the "issue" is

One "issue" of the original Reprepro source code is that many header files must be included in a certain order because they rely on the earlier header files to include other header files that they need.

For example, if the header file `A.h` needs to include a standard header file, say, `<stdlib.h>`, but `<stdlib.h>` has been included by `B.h`. Instead of including `<stdlib.h>` in `A.h`, Reprepro code always includes `A.h` after `B.h`:

```c
#include "B.h"
#include "A.h"
```

If we want to order the header files alphabetically and thus move `A.h` before `B.h`, the compiler will likely to report some missing definitions that can be found in `<stdlib.h>`.

## Is it really an issue?

I quoted the word "issue" because I don't think this is really an issue. Having read the code a bit more, I believe I understand the author's intent: try to minimize the unnecessary file reading by the preprocessor.

Typically, we define a multiple-inclusion protecting macro at the beginning of a header file:

```c
#ifndef REPREPRO_ATOMS_H
#define REPREPRO_ATOMS_H

/* ... */

#endif
```

However, this doesn't stop the preprocessor from opening the header file and reading the entire content of it. In fact, for the second and all the subsequent inclusion of this header file, the preprocessor will open and read the file, then notice the protector macro has been defined, and thus discard the content and move onto the next file.

A smart preprocessor may not have to read the whole file to find the protector macro, but I believe it's unavoidable to at least open the header file.

So the author of Reprepro is trying to minimize this kind of unnecessary file opening and reading by checking the protector macro before the `#include` statement:

```c
#ifndef REPREPRO_ATOMS_H
#include "atoms.h"
#endif
```

If the protector macro has already been defined, then the preprocessor will not process the `#include` statement at all, thus also avoid the unnecessary file opening and reading.

## My thoughts

I believe the original style does reduce the time of preprocessing and improve the performance. However, I still prefer to let data talk: Is the preprocessing time a bottleneck for building the code that optimization is needed? For example, if preprocessing takes up to a large enough percentage of the entire code building time, it may make sense to optimize it. However, I don't have the data, so I can't say if the optimization is justifiable. I do notice that building the code, with or without the file inclusion optimization, may take less than 30 seconds, so I don't feel it's very necessary to do this optimization.

## Why do I still change it?

If it's not really an issue, why do I still change it? I'm still doing this for a couple reasons.

Firstly, the author's original style is not my style so I want to make the header files self-contained so I can reorder them alphabetically.

Secondly, this repository is my own fork for learning purpose. So far, I do not have any intention to merge any code back to the original repository. So it should be OK to change the code in whatever way I prefer. If later I indeed want to submit a Pull Request, I'll fork another repository.

Last but not the least, this repository is not a piece of professional work. In a professional setting, I'll stick with whatever style that the team agree on.
