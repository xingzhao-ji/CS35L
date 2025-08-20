# Backups


**DevOps**: "Development and Operations". The idea is that you have a single staff that does both development and the logistic stuff.

The philosophy of DevOps is that the same person should be responsible of the development and the operations of the system. Historically, the communication between the two are often not that good. If the same person knows both, they'll know when and how best to optimize.

Anyway, a simple type of version control is the concept of **backups**.


## Failure Modes


Backups provide ways to recover from **failure modes**:

* **Drive failure**: the physical hardware malfunctions.
  * Total failure: the entire drive is dead.
  * Partial failure: a few blocks go bad.
  * Corrupted data with no indication (a silent and very serious error!).
* **Operational failure**: someone responsible for maintaining the software makes a mistake, like erasing data.
* **External attackers**: hackers that break into your system, encrypt all your data, and hold it ransom.
* **Data corruption**: due to power outage
* **Cosmic rays**: can cause a bit flip in the RAM or even flash on an open computer.
* etc.

> **ASIDE:** You can protect against bit flips by using **ECC RAM**. For every amount of bits, you have several **check bits** that you can use to test if a bit was flipped and correct it. All SEASnet servers and high-end work computers in general have ECC RAM. Of course, this increases the cost due to more testing required to ensure reliability.

Something to keep in mind for failure modes is not simply *what* can go wrong, but *how often* it can go wrong. This will dictate how often you should perform backups and what strategy to use.

One way to check for this is the **annualized failure rate (AFR)**. An AFR of 1% means the drive-maker thinks that if you use the drive in "the usual way" (some number of read/write operations), then on average 1% of the drives you use will fail in a year. These numbers are *estimates* based on similar drives in the past.

> **ASIDE:** **S.M.A.R.T.** is an interface you can use to interrogate a drive to determine things like how much it's been used and lifetime estimates.


## Performing Backups


**IMPORTANT:** Note that you every backup procedure should have a recovery procedure. You must also TEST your recovery procedures, or you might not be able to even restore your backup!!


### The "Simple Version"


**Backup procedure**

Periodically, make a copy of your whole *state*. Of course, this is inefficient and *doesn't scale*.

**Recovery procedure**

Simply go to the most recent copy you made and copy it *into* your state.

You could also hold onto more than one copy at a time and go to copies even earlier than the most recent one when recovering. For example, if an attacker broke in several days ago, you should probably back up to a copy *before* that and be wary of any changes after the break-in event.


## What to Back Up?


RAM + registers vs. persistent storage? Ideally you want to recover both because that's the entire *state* of your system.

In reality, RAM + registers are hard to backup because they rapidly change and because there often aren't good software interfaces for doing this. Thus, usually we focus more on persistent storage.


### Backup Levels


You can perform backups at multiple "levels", with the higher ones being more *efficient* and the lower ones being simpler and more *general*.


**Application-level Backup**

Each application knows how to back up its own data and can tailor it to its own needs.

**File-level Backup**

You save:

* Contents of the files
* Metadata of the files
* Partitioning (file system layout)

**Block-level Backup**

The above is built on top of a lower-level system that's **block** based. You want to save all the blocks representing the above.

All files are typically stored as arrays of blocks, with each blocks representing things like directory listing, actual file contents, etc. You don't have to worry about what each block represents - simply back up all the blocks and you will have backed up the entire system!

---

Practically, most systems use a combination of these levels.

**ASIDE: Why split a drive into multiple areas?**

Example 1-drive system:

```
+---+------+-------+------
| / | /usr | /home | etc.
+---+------+-------+------
```

Maybe `/` has stuff needed to boot the system, `/usr` has non-essential system files, and then `/home` has the user's documents. This way, even if there were a problem with the `/usr` segment, the system would still be able to boot, and you can then recover `/usr` from there.


The blocks are physically partitioned on the drive, with each block corresponding to a specific part of the file system.

Within the file system, there'll be a low-level representation of the file contents like metadata about who owns it, etc.

---


## When to Back Up?


**Do you bring the system down/idle to do backups?**

* If **YES**, this greatly simplifies your backup strategy. This ensures that no one's changing the blocks *while* you're doing the backup.
* If **NO**, your backup might not match any actual state because it'll be a *mixture* of some blocks from the old state and some blocks from the new state. This is sometimes called **live backups**, and this can be a real issue.

Suppose you did something like:

```shell
cd /home
tar -cf /backup/file1 *
# sometime later...
tar -cf /backup/file2 *
```

This creates a huge file `/backup/file1` containing every file under the `/home` directory. But if you do work *while* tar is working, the image that `/backup/file1` represents would be a mixture.

**More considerations: How often should you reclaim storage from backups? How many backup files do you retain?**

Obviously at least one. If the system crashes while backing up, then you just lost your only backup. Thus, you usually strive for at least two copies.


## How to Back Up Cheaply


* Do them less often.
* Do them to a cheaper device, like a hard drive or optical device.
* Do them to a remote service (this would involve some *trust* in the backup service).
* Instead of making an entire copy of your data, you do **incremental backups**.


### Incremental Backups


**Backup procedure**

Only backing up the *changes* between the old and new changes.

* The first backup is everything
* Subsequent backups are only the differences

A block-oriented backup would only need to back up the changed blocks.

**Recovery procedure**

* 1st copy: recover the full backup.
* Subsequent copies: apply the differences.

A limitation is that applying differences may take a while, so typically you would backup differences and periodically backup whole copies.

You could also *update* the backup with the differences, but this is like changing history, which introduces more concerns.

**How do you keep track of which blocks are changes?**

One approach is that with each block, you record the timestamp of when each block was changed. You'd have to record these timestamps somewhere, which means your data would be more complicated than a simple array of blocks. Some software does this anyway, where it sets aside some space for some metadata, checksums, etc.

Another option is to use the output of `diff` instead of the contents of the entire block. This is typically done at a higher level than the block level though.


## Backup Optimization


### Deduplication: Copying on Write


Suppose you execute:

```shell
cp bigfile bigcopy
```

At the block level:

```
    bigfile         bigcopy
    v               v
+---+---+---+---+---+---+---+---+
|   | b | f |   |   | b | c |   |
+---+---+---+---+---+---+---+---+
```

You could cheat by just making `bigcopy` point to the same file object as `bigfile`:

```
    bigcopy
    bigfile
    v
+---+---+---+---+---+---+---+---+
|   | b | f |   |   | b | c |   |
+---+---+---+---+---+---+---+---+
```

From the user's POV, it *looks* like a copy even if you don't have one in reality.

When you modify `bigcopy`, your `bigcopy` just "remembers" (in its underlying data structure) which additional parts are part of the copy.

```
    bigcopy --+
    bigfile   |
    v         v
+---+---+---+---+---+---+---+---+
|   | b | f | m |   | b | c |   |
+---+---+---+---+---+---+---+---+
```

This has recently become the *default* behavior of the GNU `cp` command.

This is known as **lazy copying**. Eventually, when you modify every block in your copy, *then* you will have made a copy of every block. Until then, you do it **lazily**, that is, *only as needed*.

This strategy has a name: **copy on write (CoW)**.

* ➕ Speed.
* ➕ Less space.
* ➖ Not good for *backups* because if a block gets corrupted, it affects both the original and copy.
* ➕ But this also means less underlying data when performing an actual backup.

`git clone` uses this strategy! An example of a backup at the *application level* too.


### Other Techniques


**Compression**

Often done at the block level or the file level.

You can check this. While the `-l` switch includes how many *bytes* used, the `-s` switch includes how many *blocks* are used to store the underlying file:

```shell
ls -ls file
```

**Multiplexing**

Back up many different systems to a single backup device.

**Staging**

Fast devices back up to slow devices:

```
flash --> disk --> optical tape
(fast)    (slow)   (even slower)
```

**Data Grooming**

Remove old data that you don't need anymore before backing up.

It's tedious to determine which data isn't needed anymore, so a lot of this is now done automatically. Of course, this has the downside where programs might decide to remove data that users still want.

**Encryption**

Guard against problems like attackers from the inside stealing and/or selling data to other agents.

**Checksumming: What if your backup software is flakey?**

You make checksums of your data and then back up those checksums somewhere else. When you recover, you can check if the recovered data matches the checksums.


## File Systems with Backups Built-in


Where old versions of files are always available.


### Versioning Approach


The file system keeps track of the old version of every file you create. It's as if:

```console
$ echo x > file
$ ls -l
---------------- file;1
$ echo y >> file
$ ls -1
---------------- file;1
---------------- file;2
```

**When to automatically create a new version?**

* Every time the file is opened for writing, differences made, and then closed
* Maybe there's some `newversion()` system call
* In the end, the *applications* decide when to version.

Still used in some software like OpenVMS, etc.


### Snapshot Approach


Every now and then, the underlying system takes a consistent snapshot of the entire file system.

It doesn't actually copy all the blocks because that would be too expensive. Instead, it uses CoW.

This is used in software like ZFS, btrfs, WAFL (SEASnet).
# Browsers and the Front-end


## HTML


### Tree Structure


Designed for documents accessed remotely, intended to be used by browsers - sort of like "SGML on wheels". They are both the same in that they are an ***attributed** tree of text*.

The nodes in the tree have **attributes**. The leaves of the tree are text strings.

```html
<p align="right">Hello</p>
<!-- ^ name of the ATTRIBUTE -->
<!--        ^ value of the ATTRIBUTE -->
<!--               ^ text CONTENT of the node -->
```

The HTML tree is an example of **serialization** - a standard way to convert a data structure into a string of bytes. A standard should also specify how to **deserialize** the bytes back into the original data structure.


### Terminology


An **element** is a node in the tree. This whole thing is an element:

```html
<head><!-- things here --></head>
```

The words enclosed in angle brackets are **tags**:

```html
<head><!-- things here --></head>
<!-- ^ opening tag           ^ closing tag-->
```

A **self-closing element** does not have any content, and it can be abbreviated like so:

```html
<br />
```

A **void element** is an element that never has any subtrees. These do not have to have a slash in the tag:

```html
<meta charset="UTF-8">
```

A **raw text element** is an element that can contain only text:

```html
<verbatim>lorem ipsum</verbatim>
```

A **normal element** can have sub-elements:

```html
<div>
  <p>hello there</p>
</div>
```


### DTD (Document Type Declaration)


**How do you know the context of an element's syntax?**

Some points of concern regarding the syntax of an element include:

* void, normal, raw text?
* what attributes are allowed?
* what restrictions on sub-elements?

This context is supplied by a **Document Type Definition (DTD)**, an idea inherited from SGML.

**The Problem with DTD:**

The rapid evolution of browsers prompts the continued creation of new DTDs for new classes of browsers.

The client-server model makes this a mess because the client and server need to agree on the interface between the two. DTDs need to be contracts/protocols for *interoperability* between the browser writer and the server writer.

Historically, there grew versions 1 to 4 that followed the **Internet RFC**. This process eventually broke down because there was too much bureaucracy in putting out a new DTD version.


### HTML5


A new, *evolving* standard was created, **HTML5**. This standard is edited sporadically/constantly overseen by a committee.

HTML5 does not just cover text. It also covers the intended meaning of the element, not just the layout/presentation.

It's as if it's written:

```
<p> ... </p>
"This denotes a paragraph."
```

The exact same HTML document can appear differently on different screens depending on how the browser chooses to **render** it. This is advantageous because it allows pages to have **responsive** format depending on the device they are rendered on. HTML5 separates the presentation from the styling.

> **ASIDE:** Many big companies have an incentive to join the consortium overseeing the development of projects like HTML5 because that gives them say in what happens and prevents a single entity from changing things in a direction that only benefits them.


### DOM (Document Object Model)


HTML5 also specifies the **Document Object Model (DOM)**, the standard way to access the tree representing HTML in an object-oriented program. It is like the API for tree manipulation.

The DOM unifies the stack:

* It is most often it is used inside the browser. Code inside the browser can look inside the tree.
* The server can also use the DOM to figure out what tree to build before serializing it and sending it to the browser.
* Intervening networks like routers between the client and server can also use the DOM.


## JS (JavaScript)


By default, there is a DOM **binding** for JavaScript, meaning you can traverse the tree within the programming language.

**PROBLEM:** JavaScript is a pain because it is *too powerful*, besides being an annoying language in general (very not biased). Any code can be put into a browser and introduce bugs, portability issues, etc. The debugging becomes difficult. This led to the introduction of [CSS](#css-cascading-style-sheets).

> **HISTORICALLY:** JavaScript is simple and interpreted, and was chosen primarily because it happened to be a suitable programming language during a time where one was much needed to support the growing Internet. It was originally written in about a week. Had Python or another language been ready and available at the time, it may be a different story today.


### In the DOM


JavaScript's key notion is that it is hooked into HTML. JavaScript can be written directly in the DOM:

```html
<script>console.log("hello world")</script>
<script src="myscript.js"></script>
```

The `src` attribute specifies the file to fetch. If it does not begin with a protocol (e.g. `http`/`https`), then it is resolved as a file local to the website domain.

The code within the `<script>` element has full access to the DOM, including:

* Examining the DOM
* Modifying the DOM
* Performing actions (e.g. `alert`)

This entails writing a lot of code, which makes it error-prone. Thus, **JSX** was created as a way to simplify the generation of DOM from JavaScript code.


### JSX (JavaScript Extension)


Used in frameworks like React.

Uses a **syntax extension** to JavaScript. JSX code needs to be mechanically transformed to vanilla JavaScript before it can be interpreted.

This is a common pattern in software called **preprocessing**, where code is first written in a more understood fashion before being converted to its functional equivalent.

An angle bracket `<` at the start of an expression is invalid in vanilla JavaScript, so `<expression>` denotes the departure from normal JavaScript and the start of a JSX expression:

```jsx
const language = "en";
const class_ = "CS 35L";
const header = (
  <h1 lang={language}>
    {class_} assignment {n+1}
  </h1>
);
```

Likewise, curly braces `{}` are invalid in vanilla HTML, so encountering an expression like `{expression}` denotes an embedded return to the JavaScript world. The expression enclosed in the braces can be any valid JavaScript, with the same access to the file namespace and DOM like vanilla JavaScript.


## CSS (Cascading Style Sheets)


Designed as a **declarative** spec for what appears on the screen. **Declarative** means we specify what we want, not how to get it. Generally designed more for the **web designers** on a team than the **developers**.

CSS is a compromise between the presentational layer of HTML and the interactive layer of JavaScript. With these three languages we now have the modern **separation of concerns**:

* **HTML:** Presentation (Structure)
* **CSS:** Styling (Appearance)
* **JavaScript:** Interactivity (Functionality)

CSS statements are put into DOM style elements. The styles are *cascading*, meaning by default they are inherited by subtrees.


### Competing Style Sources


There are competing sources for putting styles into the DOM tree:

1. Author of the webpage.
2. The user of the browser.
3. The browser configuration (phone, laptop, etc.).

CSS combines these sources by specifying rules for combination. In general, it is hierarchical, with the author's styles overriding the user's, which overrides the browser's, etc.


## Web Browsers


### Browser Rendering Pipeline


Abstract model:

```
HTML doc --> Browser[..., parse, ..., execute, ...] --> screen pixels
```

The browser will:

1. Parse the incoming HTML document.
2. Build up the DOM.
3. **Render** (turn into pixels) the DOM onto the screen.


By default, this is very slow, so browser writers employ various techniques to make rendering as fast as possible.


### Rendering Right Away


One solution is to make the browser start rendering before having complete information about the webpage.

In practice the HTML document is received in packets, so the browser can already read part of the document while the rest is still incoming. The root tends to arrive first, which can be parsed right away to load metadata the browser can use to deduce many details about the document.


### Optimization Techniques


1. The browser can guess whether an element will be rendered onto the screen (as opposed to being hidden at the bottom of the page outside the viewport). If not, skip it for now.

  > This means that as a JavaScript developer, you cannot assume that code will always be executed immediately because the part of the document containing the script might not be loaded yet if ever.

2. The browser may also decide that some elements are low priority, in which case, defer execution.

3. The browser can guess the overall layout of an element (like a `<table>`) and render it based on its guess. If the guess is wrong, re-render.


## Data Interchange Formats


These are standards to represent a tree structure in a compact format suitable for transmitting over networks.


### JSON (JavaScript Object Notation)


The standard way to represent a tree structure over the Internet.

**EXAMPLE:** The JSON representation of a popup menu:

```json
{
  "menu": {
    "id": "file",
    "value": "File",
    "popup": {
      "menuItems": [
        {
          "value": "New",
          "onClick": "CreateNewDoc()"
        },
        {
          "value": "Close",
          "onClick": "CloseDoc()"
        }
      ]
    }
  }
}
```


### XML (Extensible Markup Language)


Like HTML but designed for data.

<!-- I'm not even sure if this is correct syntax. I don't know XML. I just copied it from the board during lecture. -->

**EXAMPLE:** The same popup menu from the JSON example:

```xml
<menu id="file" value="file">
  <popup>
    <menuitem value="New" onClick="CreateNewDoc()">
    <menuitem value="click" onClick="CloseDoc()">
  </popup>
</menu>
```

Different network protocols can use different formats.

In general, JSON tends to be more popular. With JavaScript code, it's easier to parse strings in JSON fields than in XML attributes because JSON by design is already valid JavaScript. JSON is also slightly smaller in terms of file sizes.
# Build Tools, Makefiles, and Dependencies


## Audience for Build Tools


* **Coders:** These people write the actual source code.
* **Builders:** These people are in charge of assembling the pieces of the program into a coherent application. They "run make".
* **Distributors:** These people take the built products and ship them out to the users. They "run Debian".
* **Installers:** These people take the output of the distributors and install them on the machines.
* **Testers:** These people try to find problems in the code or any of the steps after it like distribution and installation. These are often done by people lower in the chain, like interns.
* **Users:** These people are the intended consumers of the product.


### No Build Tools?


You write your own project-specific build tools. Suppose you have a simple example of multiple C files that you need to link together. You could write a build script `build.sh`:

```shell
gcc -c x.c
gcc -c y.c
gcc -c z.c
gcc x.o y.o z.o -o foo
```

> Your goal as a software developer is to automate yourself out of a job. You constantly want to take the things that you're doing and automate it. **- Dr. Eggert, probably**


## Make and Makefiless


A simple build tool intended to be a level up from a shell script is **Make**. This was motivated from scripts taking too long to execute. They were inefficient because they would compile the entire program even if only one file was edited, so it does a lot of unnecessary work.

Thus, the essence of the **Makefile** is that you write a set of **recipes** in it, each of which instructing how to build a file. When you run `make`, the program does the *minimum amount of work* necessary to build the file. It does this with the concept of **prerequisites**.


## Anatomy of a Makefile


Make is like a hybrid language. The `target: prerequisites` lines are its own "Makefile" language. The recipes are in the shell language, commands that you would write at the terminal:

```makefile
CC = gcc
x.o: x.c
  $(CC) -c x.c
y.o: y.c
  $(CC) -c y.c
z.o: z.c
  $(CC) -c z.c
foo: x.o y.o z.o
  $(CC) $< -o $@
  # Escaping the $ so the shell can see it
  echo $$PATH
```

<!-- From discussion notes. -->

Makefiles support special variables within a rule: `$@` is the target of the rule and `$^` is the prerequisites of the rule. For example:

```makefile
foo: foo1.o foo2.o foo3.o
  g++ $^ -o $@
```

Adding a `-` before a command means to keep going even if that command fails. This can be useful in cleaning commands, like:

```makefile
distclean: distclean-recursive
  -rm -f $(am_CONFIG_DISTCLEANFILES)
  -rm -f Makefile
```


### Macros


<!-- Also from discussion. -->

```makefile
XYZ = foo1.o foo2.o foo3.o  # XYZ is a macro
foo: XYZ
  g++ XYZ -o foo
```

Commonly you'll see people define `CC` for the compiler command (`gcc`) and `CFLAGS` for the compiler options:

```makefile
CC = gcc
CFLAGS = -O2 -g3 -Wall -Wextra
```


### Phony Targets


<!-- Also from discussion. -->

A **phony target** is a rule that has NO output file. The most common example is the `clean` pattern:

```makefile
clean:
  rm *.o
```

Then use `.PHONY` to declare a phony target:

```makefile
.PHONY: clean
```


## Running Make


Make is smart enough to see the dependencies (prerequisites) and avoid doing work that has already been done. When you run:

```shell
make foo
```

It looks at the timestamp of `foo` and the timestamps of its prerequisites, `x.o`, `y.o`, `z.o`. to determine what files have changed since last run (last modified time of each file). If a file is up-to-date, it doesn't need to touch it.


You can run a specific rule to run by specifying the target at the command line:

```shell
make z.o
```

You can override macros at the command line with similar `=` syntax:

```shell
make CC=clang
```


## Common Mistakes


Often times, bugs arise from specifying the *dependencies* of the targets.

* A *missing* dependency means the product may be wrong. Make will falsely assume its job is done if it does not know if needs to act on a certain file.
* An *extraneous* dependency is a more benign mistake. Files are still compiled the same way as before, but it may cause Make to do extra work.


## Parallel Makes


Suppose you have a big project.

```
project/
  Makefile
  sub1/
    Makefile
  sub2/
    Makefile
  sub3/
    Makefile
```

One approach is to have Makefiles in each subdirectory. In this case, the root Makefile *delegate* work to the subsidiary Makefiles:

```makefile
test:
  # not sure if this is right syntax
  for i in a b c; do; (cd $$ && make); done
```

* ➕ This modularizes the build process.
* ➖ However, the parent Makefile is **sequential** code, and you don't have the opportunity to run multiple rules in parallel.

Another approach is to have one big Makefile at the project root will all the rules, including tests for anything under subdirectories. Nowadays, this is the more common pattern:

* ➕ It is a performance bonus, and it is usually automatically generated anyway.
* ➖ Extraneous/incorrect dependencies may slow down the process because there's less opportunity for parallelism. A missing dependency is even worse because they can cause different builds affecting different files, resulting in crashes.


## Building Makefiles


Suppose you have code that you want to tailor to a certain implementation. In this particular example, the C header `<stdckdint.h>` is not on SEASnet yet, but suppose you want to be able to compile code that uses it anyway.

A common convention is to have a shell script `./configure`, which *generates* Makefiles from a template, similar to the concept of a [*compiler*](Compilers.md).

<!-- I'm not sure I caught everything and/or understand everything that I copied from the whiteboard below. -->

```shell
# This syntax here is called a heredoc
cat > checkfile.c << EOF
<-- code that uses the feature -->
EOF

if gcc checkfile.c; then
  echo "#define HAVE_STDCKDINT_H 1" >> config.h
fi
```

Then in your source code:

```c
#include <config.h>
int main(void)
{
  #if HAVE_STDCKDINT_H
    use the features for stdckdint.h
  #endif
}
```

Then you define the `builder` script to first *make* the Makefile and cause other configuration side effects, and then run the Makefile:

```shell
./configure
make
```


### Autoconf and Automake


**Autoconf** is a program that generates `./configure` files. Just as how Makefiles are automatically generated by `./configure`, the `./configure` file is generated by Autoconf. We see that build tools often build on top of each other, each generated by the next. This is a common pattern in software construction.

**Automake** is like a front-end abstraction for Autoconf, so like yet another level up from Autoconf.


<!-- Below from Week 10. Thought it made sense to put them here since they're usually related to Make and Makefiles. -->


# Dependencies


You can think of them in terms of **dependency graphs**, which are typically (and ideally) *acyclic*.

Two ways dependencies come up in software construction:


## Build-time Dependency


"File x depends on file y." This can be expressed simply in Makefile syntax:

```makefile
x: y
    # create x; y is input
```

As a project scales, the dependency lists can get much longer.

```makefile
foo.o: foo.c x.h y.h z.h
    gcc -c foo.c $^
```

Maybe you extend the list based on what you see from the `#include` lines in the source files.

This can be problematic because you need to make sure that the dependencies in the Makefile *match* those of the source code, else it would slow down your building at best and break your it if a dependency is missing.

Thus, instead of maintaining dependencies *by hand*, you should *calculate* them with some `make` preprocessor - a tool to automatically generate your Makefiles for you.


## Packaging Dependency


Each package can be installed independently, BUT:

* Packages can depend on other packages
* Pnd every package can be version-specific.

This can get complicated, so ideally you'd want a **package manager**.

You can view the dependencies of a program, using `grep` as an example:

<!-- Special thanks to Victor Chinnappan for telling me what were the commands Dr. Eggert used. I didn't attend lecture that day and Bruincast didn't raise the camera. -->

```console
$ dnf deplist grep
Not root, Subscription Management repositories not updated
Last metadata expiration check: 1:16:41 ago on Mon 05 Dec 2022 04:57:02 PM PST.
package: grep-3.1-6.el8.x86_64
  dependency: /bin/sh
   provider: bash-4.4.20-4.el8_6.x86_64
  dependency: /sbin/install-info
   provider: info-6.5-7.el8.x86_64
  dependency: libc.so.6()(64bit)
   provider: glibc-2.28-211.el8.x86_64
  dependency: libc.so.6(GLIBC_2.14)(64bit)
   provider: glibc-2.28-211.el8.x86_64
  dependency: libc.so.6(GLIBC_2.2.5)(64bit)
   provider: glibc-2.28-211.el8.x86_64
  dependency: libc.so.6(GLIBC_2.3)(64bit)
   provider: glibc-2.28-211.el8.x86_64
  dependency: libc.so.6(GLIBC_2.3.4)(64bit)
   provider: glibc-2.28-211.el8.x86_64
  dependency: libc.so.6(GLIBC_2.4)(64bit)
   provider: glibc-2.28-211.el8.x86_64
  dependency: libc.so.6(GLIBC_2.5)(64bit)
   provider: glibc-2.28-211.el8.x86_64
  dependency: libpcre.so.1()(64bit)
   provider: pcre-8.42-6.el8.x86_64
  dependency: rtld(GNU_HASH)
   provider: glibc-2.28-211.el8.i686
   provider: glibc-2.28-211.el8.x86_64
```

And the dynamically linked libraries:

```console
$ ldd $(which grep)
linux-vdso.so.1 (0x00007fff5ab88000)
        libpcre2-8.so.0 => /lib64/libpcre2-8.so.0 (0x00007f365a1fd000)
        libc.so.6 => /lib64/libc.so.6 (0x00007f3659e38000)
        libpthread.so.0 => /lib64/libpthread.so.0 (0x00007f3659c18000)
        /lib64/ld-linux-x86-64.so.2 (0x00007f365a481000)
```

We notice that for example, `grep` requires `sh` because `egrep` utilizes a *shell script*. `grep` also needs the C library.
# Cloud Computing


> **ASIDE:** It's called "the cloud" because from the user/front-end's perspective, the "server" is some hazy concept that is no concern to them. They simply send requests to some back-end that dutifully processes them and respond with the desired information - it doesn't matter where or how they did it.


## As a Marketing Term


One standard definition is from the **NIST**. The capabilities of cloud computing include:

1. On-demand self-service: let you run stuff on their computers. This sets a **low barrier to entry**.
2. Rapid elasticity: you can immediately ask for more capacity. This provides **scalability**.
3. Resource pooling: share hardware resources among different projects. This **lowers cost**.
4. Measured service: the provider will bill users based on their usage. This instructs users how to **control their usage**.
5. Broad network access.


### Problems


* You have to *rely* (*trust*) that the cloud service provider fulfills their promises.
* Security and privacy.
* Providers have to make profit off you! If you're doing big computations, it's better for you to do them yourself on your own machines.


## Other Marketing Terms


From lowest level up:


### IAAS (Infrastructure as a Service)


The lowest level cloud service approach. The provider gives you ["access"](#virtual-machines) to some hardware and the network to connect to the hardware.

The hardware will be some server with some architecture (x86-64, ARM64, etc.). You can pick the operating system, and from there, you have a lot of control.

An example is the original AWS.


### PaaS (Platform as a Service)


The provider gives you access to:

* An operating system
* Architecture
* Middleware
* Database servers

An example is the Google App Engine.


### SaaS (Software as a Service)


The provider will give you access to all the above as well as all the software. You get the **full stack**.


### Serverless Computing


**SCENARIO:** You don't have to worry about the hardware or software. You don't *care* about what machine your application is running on - a level of abstraction.

"Serverless" comes from the notion of *not* maintaining one's state in RAM. Everything must be stored in the provider's persistent storage.

All the provider gives you is APIs you can interact with in **stateless programs**. These are applications that run short, fast computations when triggered by some external request, immediately save their state in the provider's database, and exit.

> In some sense, serverless computing is to server-side applications as event-driven programming is to client-side applications. **- Dr. Eggert**


## Virtual Machines (VMs)


Building blocks for the *aaS systems.

Virtual servers are slower, but a lot cheaper. There are a few ways to build a VM:


### Method 0


Write a program, typically in C, that implements the instruction set of the architecture you're simulating.

```c
char *ip;
switch (*ip++) {
    // (these are made-up numbers)
    case 0x72:
        /* ADD instruction */
        break;
    case 0x73:
        /* SUB instruction */
        break;
    /* etc. */
}
```


### Method 1


You assume that the virtual machine and physical machine have the same instruction set. You let your program run on the actual hardware.

This can be problematic because the program can reach into the physical machine and modify it however it likes. We solve this by separating the instructions into two classes:

1. **User mode** instructions: the most popular instructions like `add`, `mul`, `mov`, `call`, etc.
2. **Kernel mode** instructions: more rare ones forbidden to the user, sensitive operations like I/O, permissions, etc.

Make it so that if the virtual machine attempts to execute a kernel mode instruction, it *crashes*. When it crashes, the physical machine takes over and figures out what to do and substitutes the action with something else.

The virtual machine could be running at full speed, but when it tries a kernel instruction, it **traps** into the kernel, and the kernel decides how to handle it and returns back to the VM.

A common technique is **virtual memory**, which maps virtual addresses to physical addresses. One scenario is when a virtual address doesn't map to a valid physical address, or one that doesn't belong to you, in which case the kernel takes over and most likely gives you the infamous `SIGSEGV` :D

With this approach, you can have different systems, like a MacOS VM atop a Linux PM.

Cloud service providers can do this too.

Each VM *thinks* it has the full capability of a real machine. Of course, there's a performance problem

* The entire VM OS needs to boot.
* You'll need the resources to run every individual VM.
* If they're all running computations, they'd be independently consuming physical RAM.


### Method 2 (Containerization)


You assume that the virtual and physical machine have the same instruction set like before, but also that the virtual OS is the same as the physical OS, say Linux.

Consider the read-only file `/usr/bin/sh` sitting in the physical Linux system. All Linux VM instances can share this same shell instead of having identical copies. The VMs can even share the cached copy of the shell. This works because the file is read-only, and this applies to all other read-only files like the C library.

With this approach, you should be able to boot up a VM in less than a second.

> **Spy vs. Spy ASIDE:** Virtual machines are designed to make it very difficult to tell programmatically if you're in a VM. This guards against attackers attempting to check if they're running in a VM. One way to do it anyway is to use performance checking.


## Brief Docker Introduction


One of the most popular ways to containerize [(Method 2)](#method-2-containerization).


### dockerd


You start up a program called **dockerd**, the Docker daemon, which runs as `root`.

**ASIDE:** A **daemon** is like a behind-the-scenes server. The name is taken from Maxwell's daemon, a tiny creature that reverses entropy. According to Wikipedia:

> a daemon is a computer program that runs as a background process, rather than being under the direct control of an interactive user. Traditionally, the process names of a daemon end with the letter *d*...

The dockerd manages Docker **containers**, which are processes that correspond to some lightweight VM.

You make requests via the Docker Engine API. You can do so through the **command-line interface (CLI)**:

```shell
docker # options here
```


### Docker Objects


* **Containers** - environments to run applications.

* **Image** - a template to *build* a container, which contains an instructions on how to set up and run the container, including what files to be visible, what files to execute, etc.

* **Service** - lets applications run across container boundaries.


### Registries


Like repositories for Docker images.

You can **push** an image into a registry. You can then **pull** an image from a registry.


### Managing Docker


* Docker itself has a set of tools for managing containers, images, etc. like Docker Swarm.
* There's also **Kubernetes (K8s)** - a software generalized for other containerization.
* Amazon has its own service with ECS.
* RedHat has one too, OpenShift.


### General Issues

* **Load balancing**: distributing computation across many Docker instances running across multiple physical machines.

* **Resource constraints**: making sure your collection of containers doesn't go over your budget or your machines' physical limitations.

* Logging, monitoring, and debugging. You can even run tools like `gdb` and `strace` (both in and out of the VM) on your bad processes.

* Security, as usual.

* Deploying updates.

  * There's a technique called **blue green deployment**. As you update from "blue" to "green", you first run some green containers. You then have for some time blue containers and green containers running at the same time, and eventually you kill off the blue containers such that only green containers remain.
# How Programs Run and GCC Internals


We can divide how we execute programs into two main categories: **interpreters** and **compilers**. There also exists a compromise between these two approaches that uses **byte-code**.


## Interpreters


The **interpreter** itself is some program (compiled from some language like C in the case of CPython).

**BASIC IDEA:** Take the source code, check it a bit (validating syntax, etc.), then translate it as quickly as possible into an intermediate form on RAM.

Suppose you have this Python source code:

```python
def dist(x, y)
    return sqrt(x*x + y*y)
```

One approach for the translated form can be some symbol table with symbols pointing to tree structures, which are representations of the source code divided into elementary parts, like the function name, the arguments, the return expression, etc.

The interpreter will then walk through the data structure and evaluate the nodes. It's as if the source code of the interpreter has some kind of function like:

```c
value_t evalexpr(tree_t t) {
  switch (t->type) {
    case PRINT_STMT:
      value_t v = evalexpr(t->printargs);
      printf("%s", v);
    /* ... */
  }
}
```

Notice that the interpreter needs to be highly recursive, evaluating sub-expressions before completing a given expression.

Many interpreters use this approach, but due its recursive nature, it's a lot slower compared to machine code.

* ➕ Usually very flexible and easy to read and write.
* ➕ Very short translation time, allowing you to update and run code inconsequentially.
* ➖ Much slower at runtime compared to byte-code or machine code.
* ➖ No compiler means no compile-time checking, so errors will occur at runtime.


## Byte-code


With this approach, the program (typically still called an "interpreter") translates the source code into byte-code form. The `dist` symbol representing the function above then becomes like a pointer to some stream of operations:

```
push x
dup
*
push y
dup
*
*
+
sqrt
```

Because this is simply some byte stream, the interpreter can maintain some instruction pointer and *iterate* through the stream, executing the operations sequentially in similar fashion to true machine code. The C code now looks something like:

```c
switch (*ip++) {
  case DUP:
    top = *sp;
    *--sp = top;
    break;
  /* ... */
}
```

We've seen this exact concept back when we learned about Emacs Lisp byte-code compilation. The tree structure is like the `list` and `cons` model, and the byte-code is a sequential byte stream.

Other languages that use this approach include Python and, although not required by the spec, JavaScript on most browsers. Bash may also be another one.

* ➕ This form also produces *smaller code*, which fits into the data cache better. Tree data structures take up more space because they have a bunch of pointers.
* ➕ This form is also more *performant*. There's no recursion, simply iteratively referencing/derefencing pointers.
* ➖ Byte-code takes longer to produce compared to tree structures in fully interpreted languages.
* ➖ The tree structure approach is also more debuggable because you can trace through the tree to find what went wrong.


## Compilers


Abstractly, like a *function* that converts source code to machine code such that if you execute the machine code, the behavior will be identical to what is expected by the source code language spec.

* ➖ Translation time is large.
* ➖ Much less debuggable. Machine code is not human-readable, so we typically have to resort to debuggers like GDB.
* ➕ Much more performant at runtime. Compiled languages skip the necessity of `switch`-like code or even a data stack;they can keep their data in registers instead of plain old RAM.
* ➕ Compilers help catch errors at compile-time via mechanisms like warnings and static checking.


## JIT Compilation


A typical optimization that's done these days is like a compromise between byte-code and compilers, the concept of **runtime compilers** (aka **just-in-time (JIT) compilers**).

What it does is produce byte-code and/or possibly even tree structures, but the compiler will also have a subroutine that can compile that byte-code into machine code. The basic idea is to compile only the code that gets run a lot.

* ➕ You get the developer benefits of writing in a more flexible language with the performance benefits close to that of byte-code/machine code.
* ➕ The code "speeds up" as it runs, compiling as it goes. The subroutine can even run in a different thread.
* ➖ You need to write a subroutine for every machine the code will be run on because machine code is machine-dependent.

**Why bother with traditional compilers like those for C/C++ if JIT is so awesome?**

One catch is, who's going to write the subroutine? Another catch is, who's going to run that code?

JIT interpreters thus have some software overhead because they need to embed a compiler like GCC inside themselves.


## Compiler Portability


There are three "platforms" of concern, with specific vocabulary:

1. **"Host":** What platforms can GCC **run** on? e.g. x86-64, ARM, RISC-V, etc.
2. **"Target":** What platforms can GCC generate code **for**? You can run GCC on one architecture to generate code for another platform, so we have this distinction.
3. **"Build":** What platforms do you **build** GCC itself on?

If one wanted to build Emacs/Python, one needs to write source code in C that then translates it to machine code.

At the lower level, GCC can compile itself. The most popular compiler to build GCC is GCC itself.

**Chicken or the egg? Circularity?**

The first GCC compiler was compiled with an earlier C compiler, which in turn was compiled with assembly language. If you keep going back in time, people bootstrapped from machine code, working at the level of opcodes while toggling physical switches.


### GCC Target Assumptions


* Flat address space, where every pointer is the same width and can address any part of memory
* Support for 8-bit bytes and 16-bit words. 32-bit, 64-bit, etc. are technically not required.


## GCC Source Code


You want to modularize your compiler. You don't want to have a separate compiler for each platform. The GCC source code can be split up into machine-independent and machine-dependent parts. The latter would have separate modules for RISC-V, ARM, x86-64, etc. The majority of the code is shared code that can be consulted on any architecture.

There's also the concern of language support. GCC can compile C, C++, Java etc., so within the machine-independent part of the code, a portion is modularized into language-dependent code.

The machine-independent, language-independent part of the source code can be thought of as the "heart"/"core" of GCC. Ideally, you want as much of the source code as possible to be in this category.

```
+------+-------+--------+
|  C   |       | RISC-V |
+------+  GCC  +--------+
| C++  | heart |  ARM   |
+------+       +--------+
| Java |       | x86-64 |
+------+-------+--------+
(machine-indep) (machine-dep)
```


### Files

The heart of GCC are a bunch of `.cc` files:

```
foo.cc
fob.cc
...
```

Then there a bunch of **machine description files**:

```
x86-64.md
arm.md
...
```

These describe the machine's registers, instructions that operate on the registers, what the instructions do, etc.
The descriptions are Lisp-ish but also kind of a hybrid between ASM and C++.

For each platform, there's a `.cc` file like `x86-64.cc` that in effect *implements* the machine description so that the heart of GCC can consult the classes/methods defined by the `.cc` to figure out some hardware details.

The `.cc` specifies a bunch of subroutines to be written. There's a program written by the GCC maintainers that translates the `.md` files to `.cc` files.

This is an example of **project-specific tooling**.
# Compression


There are two big ideas used in compression programs like zlib, etc.


## (1) Huffman Coding


The best possible algorithm under its constraints:

1. Input is a sequence of symbols taken from a known alphabet.
2. We know the *popularity* of each input symbol.
3. Each input symbol corresponds to some bit stream.

When you have these constraints, you can assemble a table mapping input symbols to output bit strings, with lengths chosen based on how frequent they are. With ASCII characters in the context of the English language as an example:

| Symbol  | Bit String   |
| ------- | ------------ |
| (space) | 00           |
| e       | 010          |
| t       | 011          |
| a       | 100          |
| o       | 1010         |
| ...     | ...          |
| ^Q      | 111011011101 |

Notice that the least popular symbols could map to a representation that takes up even more space than their original form, but that's okay because they occur so scarcely that on average it does not hinder the total length by much.

Then, compression is simply a matter of iterating over the input symbols and performing a lookup.


### Decompressing


* **OPTION A:** You have a pre-computed table shared by the compressor and de-compressor. Sender computes the table and sends it first. This introduces some overhead in transmission, but more importantly, it has to read all of the input first.


* **OPTION B: Dynamic Hoffman tables**.

   1. Both sender and recipient start off with a table in their head that's perfectly balanced (for example, every character standing for itself).
   2. Sender then sends the first byte and updates its Hoffman table according to the byte that it already sent.
   3. The recipient does the same, and because both sides can detect the same popularity of the data they are sending/receiving, their tables are kept in sync.

  This is a little slower because they have to update the tables dynamically as they go but not by much. You also have to agree on what to do when there's a tie in popularity. Both sides need to be able to stay in lock step with each other.


### Table Generation Algorithm


Suppose you computed a table of probabilities (possibly by counting the frequencies of each symbol in some sample text):

| Symbol   | Probability |
| -------- | ----------- |
| (space)  | 0.1         |
| e        | 0.07        |
| t        | 0.05        |
| ...      | ...         |
| **SUM:** | 1.0         |

You build a **Huffman tree** to determine the bit string to assign. Review your MATH 61 notes lol.


## (2) Dictionary Coding


An approach that can be more optimal than Huffman coding by relaxing a constraint. Instead of mapping individual symbols to bit strings, you map sequences of symbols to bit strings.

For example:

| Sequence | Encoding |
| -------- | -------- |
| the      | 1        |
| a        | 2        |
| an       | 3        |
| or       | 4        |
| ...      | ...      |

You read the text, divide it into *words* (hence "dictionary" coding), and then for each word, you assign a number to it. Of course, *words* here refer to any byte streams, so they likely include punctuation marks or spaces for efficiency.

If the sender and recipient know this table, then a string can be compressed much more efficiently than when using Huffman coding.


### Decompressing


There is also **dynamic dictionary coding**.

1. Start with a trivial dictionary with 256 entries, where each word is length 1 and represented with some number from 0-255.
2. As data is transmitted, the tables on both side are updated dynamically and in the same way such that by the end, they can use dictionary coding.

Suppose you already sent a substantial chunk of data. You can send data in terms of what you already sent. For example, if you're about to send the word "French", and you've sent "French" 97500 bytes ago, you can send a pair of integers, offset and length, like (97500, 6), which the recipient can decode with their copy of the data.

There is also the **sliding window algorithm** where you have a moving range of the sent data that you can use for this approach. This is necessary because RAM has a limit.


## zlib


zlib uses both of these approaches. It uses dictionary coding to output a sequence of numbers, and then it uses Huffman coding on that sequence.

**Does compression guarantee you get something smaller than what you started off with?**

No. If this were true, than you can compress an indefinite number of times, all the way until 1 byte, which would then have to be compressed to 0 bytes under this assumption. This is obviously impossible to do without losing information.

> **ASIDE:** A **general purpose compression algorithm** has to be able to work with *any* byte sequence. Certain algorithms can be specialized for certain types of input, but software like Git needs something general purpose like zlib because it has to be able to work with any blob.
# C Programming


## Compilation Process


<!-- Paraphrased from slides. -->

1. **Preprocessor**: incorporate headers and `#define`s, expand macros, strip comments.
2. **Compiler**: convert code to assembly language.
3. **Assembler**: assemble code to object files/machine code (code is now in true binary).
4. **Linker**: produce the final executable; if other libraries are used via `#include` directives, link them first.

**NOTE:** Machine code and assembly code are different representations of the same thing. Machine code is raw binary while assembly code is a human-readable form of that binary, as if it goes through the binary and assigns each opcode sequence a meaningful name. You can view the assembly code of an object file by **disassembling** it:

```
objdump -d binary_name
objdump -D binary_name
```


## Compiling/Linking with GCC


<!-- From discussion notes. -->

Separating the compilation from the linking:

```shell
# First compile without linking using -c flag
gcc main.c def.h -c -o main.o
gcc def.c def.h -c o def.o
# Link the two objects into the target executable with -o flag
gcc def.o main.o -o main
```

While you could do this on one line like:

```shell
gcc main.c def.c -o main
```

The former has the benefit that when defined in a [Makefile](Build%20Tools.md), only the files that are updated are recompiled. This makes the process more efficient as we are not going through the entire compilation sequence for every file on every change.


## The C Language: Missing C++ Features


* Classes, objects, inheritance, polymorphism, etc.
* Structs having static data members and functions. Structs in C are strictly collections of data members (plain old data).
* Namespace control. C has a single top-level namespace.
* Overloading. There is a feature called `_Generic` that attempts a way around this, but this is esoteric.
* Exception handling. C has a system with `<setjump.h>` but it has all sort of trouble.
* Memory allocation. There is no `new` operator in C. Instead, you use the *library functions* `malloc()` and `free()`; they aren't built into the language itself.
* No `cin`/`cout`. You use IO with the library functions under the `<stdio.h>` header.


## Namespace Visibility


C has a very primitive version of namespace visibility compared to C++, and it does so with the keywords `static` and `extern`.

In C, **static allocation** is completely different from the `static` keyword. `static` defines a variable with **static lifetime**. This means the variable will have **internal linkage** (the identifier is visible only in the source file it is in).

Variables defined without `static` like `int x;` have **external linkage**, meaning another fine is allowed to declare it. They would do so with the `extern` keyword:

```c
// s.c
int x;
static int a[1000];
```

```c
// t.c
extern int x;
```

The two `x`'s are one and the same. On the other hand, there is no way for `t.c` to access the identifier the array named `a` (by *identifier*; if the array is passed by reference to a function, the memory can still be accessed).

> **TL;DR:** `static` is like "private". If you want your code to be *modular*, you can use `static` to define symbols as part of a private API that other parts of the code need not worry about. If you prefer the approach where all parts of the program know about every other part, you can avoid using `static` altogether.

**IMPORTANT:** Every file that `#include`s a file gets their own private copy of every `static` symbol. This is usually the cause of "defined but never used" warnings.

C gets pretty hairy when it comes to linking. Suppose you try to define a variable with the same name in yet another source file:

```c
// v.c
int x;
```

Depending on the flags used, you would either get:

* An error.
* A situation where the `x`'s refer to the same variable.


## Memory Allocation


There are 3 main allocation strategies:

| Strategy    | Memory Location | Lifetime                                                                      | Notes                                                                                                                      |
| ----------- | --------------- | ----------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------- |
| **Static**  | Data            | Before the program starts and will always exist until the program terminates. | Completely unrelated to the `static` keyword.                                                                              |
| **Auto**    | Stack           | As part of its enclosing function's **call frame**.                           | LIFO policy makes it cheap and simple to push and pop values as needed. Completely unrelated to the `auto` keyword in C++. |
| **Dynamic** | Heap            | Requested at runtime instead of at compile-time and must be manually freed.   | Usually using a memory manager implementation like `malloc()` and `free()`.                                                |


### Static Allocation


```c
// Statically allocate an integer
int x;

// Statically allocate an array of 1000 integers
int a[1000];

// Statically allocate an array of 1000 integers that also
// has static VISIBILITY (i.e. private to this file)
static int b[1000];
```

➖ There's a performance/lifetime argument against static allocation. Typically, a program runs faster if you use stack allocation instead of static allocation (where a part of memory is always allocated for something for the entire duration of the program).


### Stack Allocation


➕ Very cheap. The behavior of the stack allocation can be implemented at the *machine level*:

```c
int f(int n) {
  char buf[512]; // allocate 512B on the stack
  buf[0] = n;
}
```

```as
subq $512,%rsp
; ...
addq $512,%rsp
```


### Heap Allocation


In C, you typically use the `malloc` library function defined under that `<stdlib.h>` header to dynamically allocate data:

```c
// p is a pointer to a block of contiguous memory
// that is the size of 5 ints
int *p = (int *)malloc(5 * sizeof(int));

/* Code that uses that memory */

// De-allocate that memory when finished
free(p);
```


### ASIDE: Void Pointers


The special type `void *` refers to a **generic pointer**. You are allowed to cast any pointer type to a void pointer. Void pointers give you a lot of freedom, at the cost of checking. For example, attempting to return something dereferenced by the pointer is an error because C does not know what type it is:

```c
int bad_stuff(int *p) {
   void *q = (void *)p;
   return *q; // bad!
   return *((int *)q); // you gotta give it a type
}
```

You can think of a void pointer as the most free type of pointer. Its value is just the value of the memory address and nothing more. Attempting to use increment/decrement operators would modify it by a unit's worth of memory (typically 1 byte).

I guess this could be useful when you know something meaningful is at a memory address, but do not know the type associated with it. An example of this is the `malloc` function, which returns a pointer to a chunk of memory you allocated, but it doesn't care what type you treat that memory as, so it returns a generic `void *`.


## Common Problems in C


### Pointer Dangers


Of course, the fact that you must `free()` your memory after using it means that using heap allocation is potentially dangerous. It's much easier to get **memory leaks** and/or **segmentation faults** with improper use of pointers and memory management.

Once you have a pointer to something that doesn't exist anymore (**dangling pointer**), you cannot look at the pointer. That leads to **undefined behavior**. If you're lucky, the program will crash. If you're unlucky, the error passes silently and possibly messes with memory that isn't related to the operation.

**Dangling pointers:** The pattern looks something like:

```c
p = malloc(5); // allocate
free(p); // free
*p; // using freed memory!
```


### Memory Leaks


The pattern looks something like:

```c
p = malloc(5); // allocate
*p; // use
// but never freed!
```

Memory leaks are notoriously hard to trace. To catch them more reliably than with plain eyes, use tools or compiler options.


### Why Deal with malloc and Pointers?


Suppose we just *pre-allocate* all objects statically and hand them out as the program runs:

```c
obj_t table[10000];
```

* Firstly, how do you know how many objects to allocate? What if you run out?
* Secondly, what if you never need that much memory? Your code would be bloated, consuming more resources than it needs to. Program startup would also be slower.

Thus, we use the heap to *dynamically* allocate resources *as needed*.


## HW5: Refactoring


<!-- All of this is from discussion notes. -->

Benefits of refactoring:

* ➕ Readability and modularity
* ➕ Easier to debug
* ➕ Reduces compilation time (parallel compilation of multiple files)


### System Calls


A way for programs to interact with the operating system.

The concept of an **operating system** was invented to facilitate interaction between programs and the hardware. Without it, it would be much easier for programs to maliciously attack hardware or cause I/O conflicts with read/write operations.

Programs can make **system calls** *to* the operating system, and the operating system will then interact with the hardware in a well-defined way and report back with any output.


### Categories of System Calls


1. Process control `fork`
2. File management `open`/`close` a file, `read`/`write`
3. Device management
4. information maintenance
5. Communication
6. Protection


### The `write()` System Call


```c
#include <unistd.h>
ssize_t write(int fd, const void *buf, size_t count);
```

- `fd` stands for file descriptor, which could be stdout or stderr.
- `*buf` stands for buffer. This contains any data in it.
- `count` is the number of bytes to be written to a file descriptor from the buffer.
# Emacs Lisp


**LISP**: LISt processor because the source code is comprised of lists.

Developed by John McCarthy in 1958, Lisp is the first **functional language**.

**ELisp** (Emacs Lisp) is a variant of Lisp used to write and extend the Emacs application.

An example of **app-specific language (ASL)**. In some extent, it's an example of the *little languages philosophy*. Creators of Emacs took an existing language and mutated it to suit their particular problem.


## Namespace and Security


Emacs is very traditional - a flat, top-level namespace that can be prone to clashing. You have a lot of liberty in making problematic changes. This is allowed for example:

```lisp
emacs-version
; "28.2"
(setq emacs-version "100.0")
; "100.0"
(setq emacs-version "100.0")

; You can also mess around safely with scoped variables:
(let ((emacs-version "19.2"))
  (message "A %s" emacs-version)
  )
; "A 19.2"
```

Trying to evaluate a variable that's never been assigned, you get a *runtime error*, very reliably. There is *well-defined behavior*, as opposed to *undefined behavior* like in C/C++, the former which is obviously *safer*. The Emacs debugger also provides a traceback, but it reads bottom to top. To exit from the debugger, enter `C-]`

> Scripting languages thus tend to be used for exploratory programs, student codes, one-off programs, etc.

```lisp
(message "You can %s like printf!" "interpolate")
; "You can interpolate like printf!"
```


## Fundamental Data Structures


Emacs uses the same notation for programs and data. In other words, we use **data notation** to write our programs. The fundamental data structure in Lisp is the **list**, which is built from **cons**, which is just a pair of values. A list is singly-linked list of cons.

```
# This is a cons
[A|B]
(A . B)

# This is a list
[A| ]->[B| ]->[C| ]->[D| ]->[E|/]
(A B C D E)

# You can use . to write at the cons level
[A| ]->[B| ]->[C|D]
(A B C . D)

# The empty list; also used as the null terminator
()

# Thus this is equivalent to (A B C D E)
(A B C D E . ())

# Nested lists
[A| ]->[ | ]->[D|/]
        v
       [B| ]->[C|/]
(A (B C) D)
```

In Emacs, the *empty list* is like the *null pointer* - its byte representation is all 0s as well.


## Emacs Byte-code


You can load external source code into the current namespace with:

```
M-x load-file RET filename RET
```

As a solution to slow interpreting speed (due to the extensive use of pointers and dereferencing), ELisp uses **byte-codes** to compile data structures to create compact representations of a program.

Byte-code differs from machine code:

* **PRO:** byte-code is *portable* and works on any architecture as it is designed for some abstract machine that the Emacs application knows about.
* **CON:** Not as performant as machine code.

From the GNU documentation:

> Emacs Lisp has a compiler that translates functions written in Lisp into a special representation called byte-code that can be executed more efficiently. The compiler replaces Lisp function definitions with byte-code. When a byte-code function is called, its definition is evaluated by the byte-code interpreter.

The numbers are analogous to opcodes in true machine code. Each number represents a certain elementary operation, like pushing data onto stack memory, adding two values, etc.

For example, abstractly, this may be the compiled byte-code for some function:

```
  1 push a (arg #1)
 10 dup
 27 *
  2 push b (arg #2)
 10 dup
 27 *
 26 +
105 sqrt
```

Strung together they are functionally equivalent to the original Emacs function from which it was compiled, but now it can be compactly represented with a byte stream `1 10 27 2 10 27 26 105`.

This in turn is more performant than uncompiled Emacs code because it can be run directly by a byte-code interpreter, in contrast to high-level language that needs to be parsed (tokenized and semantically analyzed) before executing - going off of some scattered knowledge here, anyone feel free to correct me.

The byte-code files end with the `.elc` extension. You can compile a `.el` *source* file with:

```
M-x byte-compile-file RET filename.elc RET
```

`.elc` files can be loaded in the same way as `.el` files with `load-file`.

> Generally, you'd want to keep both the .el (so you can make future changes) and the .elc (so you get a performance gain) files on hand. But we're also talking about scripting in this class, and with the sizes of our scripts, this compiled/uncompiled speed difference is not noticeable (so bothering to make a .elc file in the first place is.. questionable. But it's good to know its purpose and that it's how Lisp works under the hood). **- Nik Brandt (Piazza)**


## The Emacs Interpreter


Is a *single-threaded interpreter*. There is only one instruction pointer (e.g. `%rip` on x86-64) at all times.

There's also a byte-code interpreter, which has some byte-code instruction pointer. Emacs is itself a C program, so under the hood it's something like:

```c
// points to current instruction in byte-code sequence
unsigned char *bcip;
```


## Customizing Key Binds


```lisp
(global-set-key "@" "abcxyz")
; Now typing "@" is automatically replaced with "abcxyz"

; This is like setting keyboad macros:
(global-set-key "@", 'what-cursor-position)
; Now typing "@" automatically shows cursor position in minibuffer
```

This could be a security hazard, because you can also include control characters. Emacs does provide an easy way to write control characters as strings:

```lisp
"\C-k" ; C-j
"^K"
```

Emacs however checks if you attempt to recurse like:

```lisp
(global-set-key "!" "!")
; After 0 kbd macro iterations: Lisp nesting exceeds ‘max-lisp\
; -eval-depth’
```


<!-- TODO: DISCUSSION NOTES BELOW -->


## Elisp within Emacs


* You can use `M-: RET` to enter an `Eval` minibuffer.
* You can also eval a line in a main buffer by moving the cursor to the end of the Lisp list and then entering `C-x C-e.`.
* You can directly enter an initial buffer without a file called the \*scratch\* buffer. You do this by pressing `q` right after starting up Emacs.

**Lisp interaction mode:**

* Move cursor to end of Lisp expression line.
* `C-j` to evaluate this line.
* Output will be shown and written to the buffer.
* Use instead of `C-x C-e` if you want the history to be saved in the buffer itself.


## The Language


**Atoms** are words that cannot be divided into any smaller parts, such as:
* Numbers: 30, #b111 (binary), #x6e3 (hexadecimal)
* Strings: "Hello", "buffer-name"


### Variables

Define and set a variable:

```lisp
defvar x 5
defvar y 5
```

Set the value of an existing variable:

```lisp
setq x 5
setq y 5
```

Local binding:
```lisp
(let ((a 1) (b 2)) ; local binding
    (+ a b)        ; body
)
```


### Expressions


The general syntax is `([Prefix] argument_1 argument_2 ...)`

Written as lists using *prefix notation*:

```lisp
(+ 1 2)
```

Recursive (nested) expressions:

```lisp
(* (+ 1 2) (+ 2 3))
```

**The `quote` Function:**

This will return the data structure itself i.e. `(+ 1 2)` instead of the result of its evaluation i.e. `3`:

```lisp
(quote (+ 1 2))
'(+ 1 2) ; shorthand
```


### Control Flow


Comparison operator:

```lisp
(= 1 2)
(> 2 2)
```

If statements:

```lisp
(if (= 1 2));
    "Yes"; will run if true
    "No";  will run if false
)
```

Here `;` is used to separate the lines of code, not only marking the start of comments.


### Functions


```lisp
(defun function-name (arguments...)
    "optional-documetation..."
    (interactive argument-passing-info) ; optional
    ; body
)
```

Example:

```lisp
(defun multiply-by-thirty-five (number)
    "Multiply NUMBER by Thirty Five."
    (* 35 number)
)

; calling the function
(multiply-by-thirty-five 2) ; 70
```

```lisp
; you can define a function to be interactive
; such functions can be called via M-x
(defun add (x y) (interactive) (+ x y)
```

> **Small bash ASIDE:** `.bash_profile` is for code to be only run once, like modifying environment variables such as `PATH`. `.bashrc` is for code to be run every time a new shell is started.


## Data Types


### Numerical Types


**Integer Type**

These are **all* valid integers:
```
-1
1
1.
+1
```

*fixnum*
* Its range depends on the machine: `M-: RET (print most-positive-fixnum) RET`

*bignum*:
* Can have arbitrary precision
* Most languages implement this with a *linked list*

**Floating Point Type**

All of these are the *floating point* number 1500:
```
1500.0
+15e2
15.0e+2
+1500000e-3
.15e4
```


### List Type


```lisp
(A 2 "A")       ; list of 3 elements
()              ; list of no elements (empty list)
nil             ; also the empty list
("A ()")        ; list of 1 element: the string "A"
(A ())          ; list of 2 elements: A and empty list
((A B C))         ; list of 1 element: a list with three elements
```

You can split lists across multiple lines:
```lisp
'(rose
  violet
  daisy
  buttercup)
```

Remember the quote is necessary because we want the list *itself* and not to evaluate its contents.

**What could be the output of evaluating any list?**

* Error message
* Nothing (returns the list itself using `quote`)
* Treat the first symbol in the list as a command and return its result `(+ 2 2)` -> `4`
* Evaluate an expression from a buffer directly with `C-x C-e` or `C-j`


### Other Types


**Function Type**

All functions are defined in terms of other functions except for a few called **primitive functions** written in C.

A **lambda expression** can be called as an *anonymous function*. This is useful because many functions only need to be used once.

**Character Type**

Uses ASCII encoding; a character in ELisp is nothing more than an integer

**Symbol Type**

```lisp
foo             ; symbol named 'foo'
FOO             ; symbol named 'FOO'
1+              ; symbol named '1+'
\+1             ; symbol named '+1'
\(*\ 1\ 2\)     ; symbol named '(* 1 2)' (you're an idiot)
```

**Boolean Type**

True is `t` and false is `nil`.


## Common Functions


* `quote` returns object, without evaluating it

    ```lisp
    (quote (+ 2 2))
    '(+ 2 2)
    ```

* `car` returns the first element in a list

    ```lisp
    (car '(rose violet daisy buttercup))
    ; rose
    ```

* `cdr` returns the rest of the list

    ```lisp
    (cdr '(rose violet daisy buttercup))
    ; (violet daisy buttercup)
    ```

* `cons` constructs lists

    ```lisp
    (cons 'I '(like lisp))
    ; (I like lisp)
    (cons (car '(rose violet daisy buttercup)) (cdr '(rose violet daisy buttercup)))
    ; (rose violet daisy buttercup)
    ```

* `append` attaches one list to another

    ```lisp
    (append '(1 2 3 4) '(5 6 7 8))
    ; (1 2 3 4 5 6 7 8)
    (cons '(1 2 3 4) '(5 6 7 8))
    ; ((1 2 3 4) 5 6 7 8)
    ```


## Exercises


```lisp
quote (1 2 3))                      ; (1 2 3)
'(1 2 3)                            ; (1 2 3)
(list (+ 1 2) '(+ 1 2))             ; (3 (+ 1 2))
(cons (+ 1 2) '(3 4))               ; (3 3 4)
(+ 10 (car '(1 2 3)))               ; 11
(append '(1 2) '(3 4))              ; (1 2 3 4)
(reverse (append '(1 2) '(3 4)))    ; (4 3 2 1)
(cdddar (1 2 3 4 5 6 7))            ; ERROR (unless cdddar defined)
```


## Customizing Emacs


Some Emacs jargon:

| Term   | Meaning                         |
| ------ | ------------------------------- |
| Point  | Current position of the cursor  |
| Mark   | Another position in the buffer  |
| Region | Text between the mark and point |

The function `point` returns the current position of the cursor as a number.

`save-excursion` is often used to keep point in the location expected by the user:

```lisp
(save-excursion
    (actions-that-modify-point))
```

**TIP:** You can use `(global-set-key KEY COMMAND)` to define a custom key bind.


### Example Customization


<!-- This was originally part of the midterm review in week4.md. -->

**Explain how to arrange for Emacs to treat C-t as a command that causes Emacs to issue a message like this in the echo area:**

*some time string I missed it lol*

Solution:

1. Define a function.
2. Use `current-time-string` to get current time, and use `concat` to concetatne time with other text
3. Make the function `interactive`
4. Use `message` for output in the echo area
   ```lisp
   (defun print-time ()
       (interactive)
       (message (concat "It is now" (current-time-string) ".")))
   ```
5. Create the `C-t` key binding:
   ```
   M-x global-set-key C-t print-time RET
   ```


## ASIDE: Concept of Pure Functions


Also known as **deterministic**, functions that have two properties:
1. Given a specific input `x`, the function *always* returns the same output `y`.
2. It doesn't modify any data beyond initializing local variables required to compute its output.

```c
// Pure
int f(int p) {
    int q = 5 * p * p;
    return q;
}

// Impure
int z;
int f(int p) {
    return p * z++;
}
```

Notice that as long as you're using or modifying a global mutable variable, your function risks not being a pure function.

Functional languages...
* Are only composed of functions
* Can't change variable state (no `p = p + 1`).
* No loops, only recursion (because the counter `i` changes its state).
* Order of execution is not important because all functions are pure, so they won't have any *side effects* by definition.
# Emacs


**Why use Emacs for things like cloud computing even in the modern day?**

Such applications are at the mercy of networking, which are orders of magnitude slower than operations on a local file system. Thus, they care a lot about **throughput** and **latency**, and terminal interfaces can deliver because they are simple and fast.


## Implementation


Emacs is built in "layers": it has a C interpreter inside it, and atop it is a Lisp interpreter. The bulk of Emacs is written in Lisp.

```
+------------------+
| Lisp code        |
+------------------+
| Lisp interpreter |
+------------------+
| C interpreter    |
+------------------+
```

The I/O devices like mouse, keyboard, and display communicate with the application through the Lisp code.

Programs typically have their own interpreters embedded in their own executables. For example, Chromium executables come with a JavaScript interpreter included in them.


## Concept of Buffers


* Emacs makes use of **buffers** to be *fast*. Buffers are just a bunch of text that live in RAM.
* Emacs (and editors in general) makes a clear distinction between what's *persistent* and what's not to balance speed and reliability. For work that is rapidly changing like when you're typing a sentence, we have very performant buffers. Only when work is ready to be saved, the application can flush the buffer to the file system in one fell swoop.

**Buffer-related Key Binds:**
```
C-x C-b                 list buffers
C-x b <buffer name>     switch to a buffer
```


### Auto-generated Files


* By convention, file names starting with `.#` indicate a symbolic link to a file that is currently being edited by another program.
* When editing a file `F` in Emacs:
  - Emacs creates a symlink `.#F` that signals other programs that the file is being edited.
  - Emacs creates a file `#F#`, a copy of the unsaved buffer for `F` as part of its auto-save feature, a safety mechanism for in case Emacs or the computer crashes. This file disappears on write.


## Editor Navigation: Moving the Point


**NOTE:** arrow keys (and their Shift and Ctrl variants) work too, but many of the movement keys are actually universal across Unix, which could be useful to know. Also, it's faster to use key binds that are in the main keyboard area instead of having to reach for arrow keys.

> Trust me, I spent multiple hours customizing and mastering my VS Code key binds over the last quarter. It's fun as FRICK.

The current cursor position is called the **point**.

**Smaller strides visualized:**
```
                         ^ C-p
                         |
C-a <----- M-b <-- C-b <-+-> C-f --> M-f -----> C-e
                         |
                         v C-n
```

At the **character level**:
```
C-f     Move point forward one character
C-b     Move point backwards one character
```

At the **word level**:
```
M-f     Move point forward one word
M-b     Move point backwards one word
```

At the **line level**:
```
C-e     Move point to end of current line
C-a     Move point to start of current line
C-p     Move point up one line
C-n     Move point down one line
```

These are especially useful to set up subsequent strokes. Common patterns include:

* `C-a C-k` to delete the entire line
* Starting new lines with `C-e RET` (below), `C-a C-p RET` (above)
* etc.

At the **sentence level**:
```
M-f     Move to end of next sentence
M-b     Move to beginning of previous sentence
```

At the **page level**:
```
C-v     Scroll down one page worth
M-v     Scroll up one page worth
```

At the **buffer level**:
```
M-<     Move point to start of buffer
M->     Move point to end of buffer
```

From **anywhere**:
```
M-g M-g NUM RET     Go to line number NUM
```

These are especially useful when setting up searches for the "first/last occurrence" of something.

At any point, you can use `C-l` (that's a lowercase L) to vertically center the point if possible. Use `C-l` again to bring the current line to the top of the viewport.


## Fixing Mistakes


It's important to memorize these few to know how to get back to familiar territory when something goes wrong:

```
C-/             Undo the last command (only if text was modified)
C-g             Quit current command or exit command minibuffer
ESC ESC ESC     Universal "get out" - quit command, window, etc.
C-x C-s         Save current file
C-x C-f         Open/return to a file, creating it if necessary
C-x C-c         Exit Emacs
C-z             Temporarily suspend Emacs (enter fg to re-enter)
```


## Help System


The help system makes Emacs a "self-documenting" system. `C-h` is the designated **help key**, which prefixes commands related to viewing documentation.

```
C-h b                           list key bindings
C-h k KEYSTROKE(S)              list one binding
C-h a <regex> RET               search for a command
M-x apropos RET <query> RET     search for a command
```


## Extended Commands


`C-x` is the designated **eXtended command key**, which starts keystroke chords for common commands, like `C-x C-f` to open a file.

`M-x` is the designated **eXtended named command key**, which focuses a command **minibuffer** at the bottom of the terminal, below the mode bar. The minibuffer is itself a buffer supporting your familiar navigation keys, and in it you can write the full name of commands.

After attempting to use the full name for a command that has a `C-x` shortcut, Emacs will tell you what the shortcut is for future use.

There is a special command that allows you to send a numerical argument to a command:

```
C-u NUM KEY
```

For many commands, this simply repeats the actions, but some commands may have slightly different behavior. For example:

```
C-u 2 C-k       delete content of two lines and their newlines
C-k C-k         delete content of a line, and then its newline
```

This behavior is determined by how they're implemented - whether they take that optional numeric argument in the first place and what they do with it.


## Window and Buffer Navigation


```
C-x 2       split current window into two
C-x o       switch focus to the other window
C-x 0       kill current window
C-x 1       kill all windows except the current window
```

Opening another window for the same buffer does not duplicate the buffer; any edits in one buffer will affect the other(s). You can still use this when you want to reference some other part of the buffer while typing in another region.

```
C-x C-b     list the current buffers
C-x b       switch to a specific buffer
C-x s       prompt save for some buffers
C-x 4 C-f   open file in another window
```


## Shell within Emacs


Run a command as a separate, one-off shell process:

```
M-! <command> RET
```

Same thing but taking input from a buffer and then piping it to the shell command (takes all characters in **current region** and pipes them to the command as its stdin, and then takes the output of the command and pipes it to the *shell command output buffer* like normal):

```
M-| <command> RET
```


## Selection Manipulation


Concept of the "current region (of current buffer)":

* You can save pointers called **marks** at arbitrary positions within a buffer.
* The **current region** is all the characters between the mark and the point.

You can set a mark at the current point with `C-SPC` or `C-@`. An example of selecting a region of text:

```
M-<             go to start of buffer
C-@             set marker
C-s eggert RET  search for "eggert"
M-|             pipe buffer into a shell command
```

You can find out where your mark is with `C-x C-x`, which exchanges point and mark (selects the text between them). You can `C-g` to cancel the selection.


## Text Manipulation


Emacs distinguishes **killing** from **deleting**. When you **kill** text, it is actually saved in a special buffer called the **kill ring**, and content in here can be reinserted (aka **yanked**) at the point. Most commands that perform bulk removal of text actually *kill* the text, not *delete* it.

```
C-k     kill from point to end of line
C-w     kill current selection
M-w     copy curernt selection
C-y     yank most recent kill
M-y     cycle the text to yank through kill history
```


### Modes


Emacs is a **modeful** editor. That means the current state of Emacs not only includes the contents of the current files being edited but also what way you intend to be using the editor next. A **mode** is like a method of interacting with the editor.

* **Upside:** more efficient for experts
* **Downside:** confusing/tricky for non-experts

```
M-x MODENAME            switch to mode
```

`C-h m` brings up a buffer that describes what mode you are in. The default "editor" mode is called **Fundamental** mode, and there is also **Text** mode in which navigating among words with certain characters like apostrophes is slightly different.

You can also open a shell subprocess (**Shell** mode) or a view of a directory (**dired**, directory-editing mode). The mode you are in affects the keys you input. You can see the name of the major mode you are currently in with the **mode bar** just above the minibuffer.

```
C-x d <dirname> RET     enter dired mode for directory
```
# Files and Filesystems


## Tree Structure


* The model popularized by Unix and Linux.
* **Directory**: file that maps *file name **components*** to files; they are literally just look-up tables.
* **Regular files**: byte sequences (can be read from or written to, unlike directories).
* **Special files**: built into the kernel e.g. `/dev/null`.
* **Symbolic links** (aka **soft links**): single files that contain a single string that is interpreted as a file name (could be any file or directory, including another symbolic link). When using `ls -l`, you can see symbolic links with `->` pointing to their **target**.
* Every file name look-up follows the pattern: starting at the root, consult the directory table, resolving any symbolic links along the way.
* A **file name component** is a nonempty sequence of non-slash characters. The `/` character is *very special* in the POSIX file convention because they are interpreted as part of a path in the tree filesystem.


> `/dev/null` is useful for "reading nothing" or discarding output:
>
> ```shell
> echo abc > /dev/null
> ```

**TIP:** The first flag for a file when using `ls -l` tells you the file type: `d` for directory, `l` for symbolic link, `c` for special files like `/dev/null`, `-` for regular files.

File names are not recursive by design. If you want to search for files that match the pattern like `/usr/*/emacs`, then use the `find` command:

```shell
find /usr/ -name emacs
```

<!-- Originally I had written "inum" in place of "inode number" because that's what I could hear during lectures. It doesn't seem like "inum" is an actual term, so I've updated that. -->

Every file in the POSIX standard has a unique **inode number** associated with it. You can see them with the `-i` flag in the `ls` command:

```
$ ls -idl /
2 dr-xr-xr-x. 21 root root 4096 Dec 17  2021 /
# 2 is the inode num
```


## Linux Filesystems


It's not *actually* a tree, but it still does not have loops - it's a **directed acyclic graph (DAG)**. They achieved this with the notion of **hard links**, which are like aliases for the same file in memory. The iron-clad rule is that hard links *must not point to directories*.

```console
$ ls -al /usr/bin
total 373832
sdafhausegulaskhglkas .
dsafhsagkjsajhgjasjga ..
sdafkjdhsajsadhgklsda '['
```

Every directory includes links to themselves (`.`) and their parent directory (`..`). These are hidden by default, but you can view these with `ls -a`. Note that the parent of the root directory `/` is itself, `/`.

File names *starting* with slash are **absolute paths**, paths that start at the root. File names that do not start with a slash are interpreted as **relative paths**, paths starting from the **current working directory**.

Remember that the file system is just data structures that are mostly on *secondary storage*, which are *persistent*, but much MUCH slower than primary storage aka RAM. Just like RAM, filesystems can have "pointers" too which reference locations on the hard drive. Thus, data structures in the filesystem have to be very intelligently designed in order to be efficient.


### Data Structure Representations


A directory is really just a *mapping* of the file names components to the inodes (via inode numbers) in the filesystem tree.

You can visualize the filesystem as a graph of nodes representing file objects in memory but the file name components along the *edges* of the graph.

The filesystem is not a tree, but there are some limitations enforced:

1. You cannot have two different parents with the same directory. The "tree-like" structure holds for directories.
2. However, you can have multiple links to the same non-file. Files are often identified with names, but actually names are just **paths TO** the file. You cannot in general look at a file in a filesystem and determine what it's "name" is because it could have multiple of them. Names are really just useful aliases that we as users assign to the actual file object.


### Hard Links


Creating a **hard link** - "create a file named b that's the same as the file already named a":

```shell
ln a b
```

This is like a *mutable reference* in C++. If you modify one, you modify the other because they're the *same file in memory* and even share the same inode number:

```console
$ ln a b
$ echo "foo" > b
$ cat b
foo
$ cat a
foo
```

Hard links work because a directory is simply a *mapping* of file name components to files. The file names are different, and there's no rule saying different keys can't map to the same value, so everything is consistent with what the definition of a directory.


### Soft Links


**Soft link (symbolic links)** on the other hand are actual separate data structures that have content (the string that is interpreted as the path to their target). A hard link only contributes to the directory size (expands the mapping by one entry). The underlying file remains unchanged.

Symbolic links can also point to nowhere. They can be **dangling**. When using such a pointer, the OS will try to resolve the existing path that's saved as the content of its file, but if that file no longer exists, then you get the error:

```console
linkname: No such file or directory
```

A symbolic link is always interpreted *when you use it*, NOT when you create it. If a dangling symlink is pointing to a non-existent `foo`, but then you create a new file `foo`, the symlink works again.

<!-- From midterm review. -->

**What is the main difference between a hard link and a symbolic link?**

A hard link increments the file object's reference count. A symbolic link does not increment the file's reference count.

Deleting all hard links to a file (and ceasing all operations that use the file) deletes the file. Deleting symbolic links do not affect the underlying file. A symbolic link can become dangling if the underlying file is deleted.


### Destroying a File


You can use the "remove" command:

```shell
rm file
```

However, this is actually an operation on the directory, not the file in memory itself. What `rm` is doing is modifying the current directory so that `file` no longer *maps* to the file object it did. The contents of the file object still exist in memory. So as a continuation from above:

```shell
rm bar
ls -ali
# foo still shows up
```

**So how does the filesystem know when to reclaim the memory?**

Recursively searching every directory for any remaining hard links would be too slow.

Instead, the filesystem maintains a reference count called a **link count**. Associated with each file is a number that counts the number of hard links to the file i.e the number of entities that map to this file.

**IMPORTANT:** Symlinks DO NOT contribute to the link count. Link counts ONLY count hard links.

You can use this to list the link count to each file, next to the permission flags:

```shell
ls -li
```

**HOWEVER:** Memory cannot be reclaimed until all processes using/accessing the file are done with it.

Operations like `rm` simply decrement the link count. **If it's 0,** there's still one step left: the OS must wait until every process accessing the file exits or closes. **Then** the OS can reclaim the memory.

Likewise, operations like `ln` *increment* the link count, for both the original file and the new hard link.

**ALSO:** Even if the link count is 0 and no processes are working, the data is still sitting in storage. It could be overwritten naturally by new files, but it is not guaranteed, and you must use low level techniques to either irreversibly remove the content or recover it.


### ASIDE: Some Filesystem Commands


The `ls -li` flag outputs each entry in the format:

```console
$ ls -l
(inode num) (file type AND permissions) (link count) (owner)
```

Listing all the filesystems and how much space is available in each of them:

```shell
df -k
```

`find` supports searching by inode number:

```shell
find . -inum 4590237 -print
```

Remove every file with that inode number found within the current directory:

```shell
find . -inum 4590273 -exec rm {} ';'
```


### Soft Link Edge Cases


**Can you have symbolic links to directories?**

```console
$ ls -li /bin
... /bin -> usr/bin
```

Yes. A symlink can even be resolved in the middle of a file name (in which case, it better be a directory).

**Can a symlink point to another symlink?** Yes.

**Can a symlink point to itself?** Yes. It can even point to a symlink that points back to itself. They aren't *dangling* lists, but if you try resolving the path, you enter an infinite loop.

The kernel has a guard against this. If you attempt this, you get the error:

```console
filename: Too many levels of symbolic links
```

**Symbolic links to hard links?** Yes.

**Hard links to symbolic links?** Yes.

<!-- This was originally part of the midterm review in week4.md. -->

**Give an example of how renaming a dangling symbolic link can transform it into a non-dangling symbolic link.**

Soft links can be linked to a relative path. Suppose `c` has content `b`, but `b` only exists as `temp/b` relative to the current working directory. We can use `mv c temp` to make the relative path `b` now work since `c` is now in the same directory as `b`.



### Secondary Storage


Flash drives wear out.

"4 TB drive 200TBW" means it has 4TB capacity and you are guaranteed that you can overwrite the same block of memory with 200TB worth of data. After that, that part of the drive wears out.

Linux solves this problem by moving the file around. The user thinks they're writing to the same area, but the OS is actually writing to different parts of the drive.

The implication is that if you remove the file, there are actually parts of the file still scattered around the drive. Attempting a clean wipe of the drive is more involved, and there is also software dedicated to recovering deleted date from these data remnants.

---

**ASIDE:** Overwriting the content of a file with random junk:

```shell
shred foo
```


## The `mv` Command


You can use the "move" command to rename and/or move a file.

```shell
mv foo.c bar.c
mov foo.c bar  # if bar's a directory, it becomes bar/foo.c
```

This is a very *cheap* operation because it actually just modifies the mapping of the directory instead of the files themselves. What this does at the low level is:

* Remove one directory entry
* Add another entry in a directory, possibly the same one or another directory

`cp` on the other hand is more expensive because it has to actually iterate over the content of the file. Although, we [learn later that that might not always be the case with the **copy-on-write** technique](Backups.md).


## File Permissions


When you run something like `ls -l` you see that the first column has a sequence of characters represented like:

```
rw-r--r--
```

The permissions bits are just a 9-bit number, which stores 3 groups of octal numbers representing the `rwx` permission bits for the `ugo` (owner, group, other) of the file. The flags displayed with `ls -l` have ten bits, with the leading bit representing the **file type**:

| Bit | File Type         |
| --- | ----------------- |
| -   | regular           |
| d   | directory         |
| l   | symbolic link     |
| c   | char special file |
| ... | ...               |

Technically, the 9 bits are actually 12 bits because they are encoded in a way to allow for the special flag:

```console
$ ls -lai /bin/sudo
... -rws-r-xr-x 1 root root ... /bin/sudo
```

The `x` flag of the octal number for the owner category is an `s`,
short for **superuser**. This means that this command is *trusted* by the OS. And no, you cannot just:

```console
$ chmod +s /bin/sh
chmod: changing permissions of 'bin/sh': Operation not permitted
```


### Sensible Permissions


<!-- This was originally part of the midterm review in week4.md. -->

A set of rwx permissions on a file is called "**sensible**" if the owner has al the permissions of the group and the group has all the permission of others. For example:

- 551 (r-x|r-x|--x) is sensible. The owner's permissions are a *superset* of those of the group.
- 467 (r--|rw-|rwx) is not. The group has `w` permission while the owner doesn't.

Non-sensible permissions don't make sense because the *owner* is considered to be the most closely related to the file, so they should have the most access.

**How many distinct sensible permissions are there?**

Solution:

Consider r, w, x permissions separately. For each of then, there are 4 ways to gst sensible permissions for u, g, and o (in binary, 000, 100, 110, 111).

Total: $4^3=64$


## So How Do You *Actually* Update a File?


Options:

1. Write directly to a file `F`. But if some other program reads the file at the same time, problems could arise. You want to be able to update files (and databases) **atomically**.
2. Write to a temporary file `F#` and then `mv F# F`. The downside obviously is that it occupies twice the space on drive. The upside is that because `mv` is **atomic**, any other processes attempting to use the file at the same time will either get the old file or the new file, not some intermediate state.


<script type="text/javascript" src="http://cdn.mathjax.org/mathjax/latest/MathJax.js?config=TeX-AMS-MML_HTMLorMML"></script>
<script type="text/x-mathjax-config">
  MathJax.Hub.Config({ tex2jax: {inlineMath: [['$', '$']]}, messageStyle: "none" });
</script>
# Git Internals: Plumbing Commands


## Plumbing vs. Porcelain


The **plumbing** part of a program is the low-level part, where the actual work is done. It is not easy to use and not meant to be easy.

The **porcelain** is the high-level commands for the end users.

Git actually violates a common software engineering rule, which is "Just show the porcelain to the outside world. Don't expose your plumbing."

Instead, Git wants to have two levels. Even at the low-level, it wants to expose its basic model for how Git works if the user is willing to understand how it works.


## Hard Links


**Why is cloning so fast on a local machine?**

If you explore your files with `ls -l`, you'll notice that the link count of some files that were cloned locally are greater than expected.

We see that a Git clone is created quickly partly because it did not have to make copies of the objects, and rather it used **hard links**.

More specifically, it makes hard links to read-only files. It knows it's not going to cause a problem because people cannot modify them, so they're safe to share.

A consequence is that most of the stuff in `.git` is stuff you cannot change. If you could, you can no longer clone it. If you try to be a troublemaker and use `chmod` and try editing such files, it could mess up other repositories using it (if that's even allowed at all).


## The .git Directory


The entire state of the repository is encoded in the special `.git` directory.

Note that there's an issue of compatibility. When a new version of Git comes out, it needs to be able to work with old repositories (to maintain **backwards compatibility**). The converse is not always true; repositories created by recent versions of Git may not necessarily work with older versions of Git. Thus, there are some files in the `.git` folder that are archaic.

Some important files and directories in the `.git` subdirectory:

* `branches` - obsolescent.
* `config` - repository-specific configuration.
* `description` - used for GitWeb, an attempt to put Git on the web.
* `HEAD` - where the current branch is.
* `hooks/*` - executable scripts that Git will invoke at certain "pressure points" (important triggers, like making a commit). By default, there are no working hooks; default ones all end with `.sample`, which illustrate what you might want to put in such hooks.
* `index` - a list of planned changes for the next commit. This is in binary data.
* `info/exclude` - addition to `.gitignore`.
* `logs` - keeps track of where the branches have been (histories of branch tip locations).
* `objects` - where the actual "repository" is, where the object database is stored.
* `refs` - where the branch tips and tags are (where all the "pointers" in the repository are).
* `packed-refs` - optimized version of `refs`.

> **Emacs Hooks ASIDE:** Because `git clone` does NOT copy hooks, fresh local copies of a repository always have no functioning hooks. Emacs maintainers went around this by preparing a script in the Emacs source code called `autogen.sh`. This script creates a bunch of Git hooks that tailor the repository to be the way the Emacs developers want it to be tailored. This is a nice "gatekeeper" approach that ensures your development is relatively clean.


## Comparison to Filesystems


It seems that Git uses a collection of files to represent a repository (and the index). In reality, Git actually uses a combination of secondary storage and RAM (cache).

A local Git repository and the index are made up of **objects** and some other auxiliary files. Git objects are like a tree of files in the filesystem.

**REMINDER:** Every file has a unique index, namely the [**inode number**](Filesystems.md#files-and-filesystems), which you can see with `ls -i`.

Analogously, SHA-1 checksums for Git objects have the role that inode numbers have in filesystems. They are comparable to pointers in C/C++, values that uniquely identify the actual objects they reference.

**DIFFERENCE:** Files in the filesystem can be mutable. inode numbers thus exist *independently* of the contents of their files. However, checksums uniquely identify objects *by their content*. Therefore, you **cannot** change objects' contents.

**SIMILARITY:** Both are directed acyclic graphs (DAGs). In the filesystem, it's guaranteed by the OS that you cannot have cycles. For Git objects, you cannot create a cycle because you can only *add* to history, not change it.


## Git Objects


### Creating Them


Generate a checksum from string content:

```console
$ echo 'Arma virumque cano.' | git hash-object --stdin
24b390b0e3489b71977f5c7242a4679287349242
```

You can also supply a file name as a positional argument instead of using `--stdin`.

Computing the checksum *and* writing it to the repository:

```console
$ echo 'Arma virumque cano.' | git hash-object --stdin -w
24b390b0e3489b71977f5c7242a4679287349242
$ # This object now exists in the file system
$ ls -l .git/objects/24/b390b0e3489b71977f5c7242a4679287349242
-r--r--r-- 1 vinlin 197609 36 Nov 22 03:46 .git/objects/24/b390b0e3489b71977f5c7242a4679287349242
```

Notice that the file has 444 permissions. *No one* is allowed to write to this file.

*But there's always a troublemaker!*

```shell
chmod u+w .git/objects/24/b390b0e3489b71977f5c7242a4679287349242
```

You're technically *allowed* to do this, but in doing so, you're violating an invariant that Git trusts in order to properly function, so live your with your consequences I guess.


### Examining Objects


Decoding the content of the created Git object:

```console
$ git cat-file -p 24b390b0e3489b71977f5c7242a4679287349242
Arma virumque cano.
```

You can check the *type* of the file with:

```console
$ # !$ is shorthand for the last arg of prev cmd!
$ git cat-file -t !$
blob
```


### Object Types


* **blob**: represents any bytes sequence, like regular files in a file system
* **tree**: represents a node in a tree of objects; maps names to SHA-1 checksums of blobs or other tree

You can see the organization in action with something like:

```console
$ git cat-file -p 'main^{tree}'
100644 blob fa86c55e687fef76cb5801776756a6cb204cd2f9  .gitignore
100644 blob 630e72d7faef82103be5f27139030b67ef942ca0  README.md
100644 blob 1c6781665caca7fa3350bdfcb4a820fbec2dab05  week1.md
100644 blob f1f8f3890378332cffb6ddfb3dbc84c20e3a9470  week2.md
100644 blob d94ba050bf8045d989617e2fdf75206b2d7d97a4  week3.md
100644 blob 1131c977a2f8db9031e081a16c028bf493dfd02a  week4.md
100644 blob 4923796df144352f4784e7a90cf58a98e64d7e04  week5.md
100644 blob 8017ea5aaff41016d6813e329f75a008d2b8cd0a  week6.md
100644 blob e34d49dc0e8754b9de4e1a46449541c15e31af86  week7.md
100644 blob 8110b211ea75e284e8ad5a55b8e97c9927a0dc8a  week8.md
```

Each commit points to a tree. The tree is like a directory in a file system. Above, `main^{tree}` refers to the tree object referenced by the commit object referenced by the branch named `main`.

Examining the output, we see 4 columns:

```
100644 blob fa86c55e687fef76cb5801776756a6cb204cd2f9  .gitignore
(mode)(type) (SHA-1 checksum)                           (name)
```

* **1st column:** octal digits, the last three of which represent the Linux permissions of the file in question (so `644` means `rw-r--r--`).
* **2nd column:** the type, **blob**, another **tree**, etc.
* **3rd column:** the SHA-1 checksum of the *referred* object. After all, a tree just represents a pointer to a bunch of other objects.
* **4th column:** the name of the file.


### Object Anatomy


Every commit object points to two things. First is the tree the commit represents. The second is the parent commit object(s).

```console
$ git cat-file -p main
tree aa3ca55b785ab21cfbbca0a89843151d389dcac8
parent 65422241d84b3087142adcf0906b5f86e69230e3
author Vincent Lin <vinlin24@outlook.com> 1668666720 -0800
committer Vincent Lin <vinlin24@outlook.com> 1668666720 -0800

Add 11/16 lecture notes
```

We see that every commit object contains information about its parent, author and committer.

You can think of `git log` (a *porcelain* command) as pretty-printing what `cat-file -p REF` (a *plumbing* command) tells us.

Three levels going on with every commit object:

```
(commit object)...
       |
       |                  +---------> (other objects)
       v                  |
(commit object) --> (tree object) --> (other objects)
       |                  |
       |                  +---------> (other objects)
       v
(commit object)...
```

Every commit points to a different tree, but the tree can share the objects they reference. When you make a commit for small changes, you don't have to rebuild a bunch of objects. Unchanged objects are *reused*.

However, because changing a file would update the tree containing it, changing a deeply nested file would require new tree objects all the way up to the project root for the new commit.

If a file is extremely large, Git can store a *diff* instead of a full copy and just remember how to restore the full content when the blob is needed.


### Form for Blobs


Suppose a blob represents some byte stream `B`, say 47 bytes long. The form it is stored in has a header that tells the type of the object, some control information like how large it is, and then the actual content:

```
+-+-+-+-+-+-+-+--+------------------------------+
|b|l|o|b| |4|7|\0|<--------------B------------->|
+-+-+-+-+-+-+-+--+------------------------------+
```

This string is then compressed using zlib, and that's the form it is stored in on the file system.

This string is also what is fed to the SHA-1 checksum algorithm.

> **ASIDE:** Mathematicians in the past decade have been working to try and crack SHA-1 and have succeeded for the most part. SHA-1 is no longer "reliable", but the chance of a *random* collision (not one with file contents engineered to hash a collision) is still astronomically small.


## The .git/objects Directory


<!-- From discussion notes, shared by Snigdha Kansal because I did not attend that week. -->

`objects` is the most important folder. It contains the commit objects, commit history, etc.

Objects are stored two levels deep. The object is named with its 40 hex-digit SHA-1 hash value. The first two digits are the name of the subdirectory it resides in, and the remaining 38 digits are the file name. For example, an object with hash value `24b390b0e3489b71977f5c7242a4679287349242` would be stored at `.git/objects/24/b390b0e3489b71977f5c7242a4679287349242`.

Most Git objects are actually zlib-compressed, so attempting to view the contents directly with `cat` or in an editor would give you gibberish. You can safely see the context of a Git object with the plumbing command:

```shell
git cat-file -p 24b390b0e3489b71977f5c7242a4679287349242
```

You can read the contents of Git objects in Python by decompressing it with the `zlib` standard library module:

```python
import zlib
object_path = ".git/objects/24/b390b0e3489b71977f5c7242a4679287349242"
with open(object_path, "rb") as fp:
    compressed = fp.read()
content = zlib.decompress(compressed).decode("utf-8")
print(content)
```


### HW6: Topological Sorting


<!-- Also from discussion. -->

We can use **Kahn's algorithm** to implement **topological sorting**. No nodes are listed first in this ordering. More specifically, for every child-parent relationship, the parent appears before the child in the sequence. The pseudocode from [Wikipedia](https://en.wikipedia.org/wiki/Topological_sorting#Kahn's_algorithm):

```
L ← Empty list that will contain the sorted elements
S ← Set of all nodes with no incoming edge

while S is not empty do
    remove a node n from S
    add n to L
    for each node m with an edge e from n to m do
        remove edge e from the graph
        if m has no other incoming edges then
            insert m into S

if graph has edges then
    return error   (graph has at least one cycle)
else
    return L   (a topologically sorted order)
```


## HW6: The .git/refs Directory


Branch names often assume the pattern of file paths because that's exactly how they're stored under `.git/refs` subdirectories.

**Local branches are under `heads/`:**

* `main` -> `.git/refs/heads/main`
* `fix/issue2` -> `.git/refs/heads/fix/issue2`

**Remote branches are under `remotes/`:**

* `origin/main` -> `.git/refs/remotes/origin/main`
* `origin/fix/issue2` -> `.git/refs/remotes/origin/fix/issue2`

**Tags are under `tags/`:**

* `v37` -> `.git/refs/tags/v37`
* `fixed/issue4` -> `.git/refs/tags/fixed/issue4`

Notice that ref names with slashes in them are stored as files nested within subdirectories accordingly. The *leaves* (last part of the path) are the actual files containing information about the branch the path corresponds to. So the files with content are the ones returned by:

```console
$ find .git/refs -type f
$ find .git/refs -type f
.git/refs/heads/main
.git/refs/remotes/origin/main
.git/refs/tags/test/tag
```

In the above example, the file named `tag` at relative path `.git/refs/tags/test/tag` encodes the branch named with the subpath `test/tag`.


### Branch File Anatomy


The files representing the branches have a singular line of uncompressed data in it:

```console
$ cat .git/refs/heads/main
608a9573dc8fc1f9fdec898ae7a81d47ec91923a
```

It's simply the 40 hex-digit SHA value of the commit at their branch tip.


## Tagging


A **branch** is a lightweight movable name for a commit.

A **tag** is a heavier-weight constant name. It's like a label that references a commit, and unlike a branch, it does not move when new commits are added. It contains:

* Name
* Commit ID
* Other metadata

Listing your tags:

```shell
git tag
```

Tags tend to be used for releases. For ordinary (unsigned) tags, the following tags `HEAD` with the ref `v38`.

```shell
git tag v38
```


### Publishing Tags


Created tags are *local* to your repository. To *publish* this tag to the outside world...

`git push` won't work because what `push` actually does is look at current `HEAD` and the branches associated with that head and pushes those changes upstream. In other words, it pushes the *current branch* upstream. Tags are just names for commits, so they're not part of any branches.

Developers may also be in the habit of using many tags, and with many such developers at the same time, automatically pushing tags with every `git push` would very quickly pollute the upstream repository. Thus, not pushing tags by default is *by design*.

You must manually push tags:

```shell
git push --tags
```


### Signed Tags


In a large codebase, chances are there will be many commits and tags that are in a bad state. Signing is like cryptographically "approving" a commit. Tag signing is used for security-sensitive software.

Creating a signed tag:

```shell
git tag -s v37 -m "Good version 37"
```

The `-s` flag signs the ref named `v37` with a message `Good version 37`. The signing creates a **GPG (GNU Privacy Guard) key**.

Other people can then check the tag to verify that it was signed:

```shell
git tag -v v37
```

We can take a Git tag, say `v37`, which references some commit with ID `09cf...`. What signing a tag does it taking the tag and signing it with a private key `K` and then publishing the result, which looks like a random bit string. Any other user can then use the signer's public key `U` to decrypt the tag.

A weakness is that if the private key `K` is leaked out, attackers can put out a bad version of the software that looks like it was signed by authority.



### ASIDE: Cryptography


**Private key system**

The simplest system, where the sender and recipient share some key `K`.

You take your message `M` encrypted with a key `K`, and that's what's published. Attackers can't directly see the contents of the data. The recipient then uses their same `K` to decrypt `M`:

```
{M}K ===> [{M}K]K = M
```

**Public key system**

You have a pair of keys `(U, K)`, a **public key** and **private key** respectively.

The sender keeps its private key `K` secret and doesn't reveal it even to the recipient. The recipient publishes its public key `U`.

The sender encrypts the message with the recipient's public key `M` and publishes it. The recipient then uses their private key `K` to decrypt the message. Because of some math magic, `U` and `K` can undo one another despite being distinct:

```
{M}U ===> [{M}U]K = M
```

This also works vice versa. The sender can encrypt the message with their private key while the recipient uses the sender's public key to decrypt it:

```
{M}K ===> [{M}K]U = M
```

This system is what the GPG *keys* in Git use.


## Reflog


```shell
git reflog
```

This inspects the `reflog` file. It is not maintained as an object in the object database. It is a *separate* log file that logs the *changes* that you made to the object database (most recent first).

`git reflog` lists the changes you make to a repository. It keeps track of things like moving `HEAD` as a result of committing, checking out, etc.

You can think of it as a "second order derivative" of your repository with respect to time, with the first order being the commit history, and the original function being your working files. Reflog is like changes *about* changes.


## Object Naming Algebra


If you read the `man` pages for any Git commands, chances are you will encounter some recurring vocabulary and syntax patterns.

Firstly, names fall into two major categories:

**pathspec**

This refers to working file names, stuff like `src/main.c`.

Example of using a pathspec in a command:

```shell
git diff foo.c
```

**commit names**

This refers to commits and their aliases, stuff like:

* The `HEAD` pointer.
* Commits relative to another ref, like `HEAD^^2~`.
* Branch names like `main`, which really refer to the commit at their branch tip.
* Tag names like `v37`.

Example of using a commit name in a command:

```shell
git diff main
```

---

These two sets have different naming conventions. Notice that for the `git diff` examples, what if there's an ambiguity, like a file named `main`? Git commands support a special argument `--` that separates the part of a Git command that talks about *commit names* and parts that talk about *pathspecs.*. For example:

```shell
git diff main -- foo.c
```

This says, "show the difference between `foo.c` in the index and the one that's in the commit referenced by `main`."

If the `--` delimiter is missing, Git will try to guess. It general, you should try to avoid ambiguity by explicitly supplying the `--` token:

The equivalent commands from the prior examples:

```shell
git diff -- foo.c
git diff main --
```


### Commit Name Notation


<!-- Now incorporating one of my Piazza answers. -->

The `^` suffix on a commit name denotes a *parent* of that commit. You can think of the `^` as an operator. `^` means "get the parent", and it can take an optional integer "argument" to specify *which* parent in the case of a merge commit. So `HEAD^` is short for `HEAD^1`.

```
N^    # parent of N, equivalent to N^1
N^2   # second parent of N
N^^   # first parent of the first parent of N
N^2^  # first parent of the second parent of N
```

You can also use `~` to automatically take the first option when the ancestors of commits happen to branch into different parents. In other words, `N~k` is shorthand for `N` followed by `k` carets.

```
N~2  # N's grandparent, equivalent to N^^
```

You can combine the notations (evaluating left-to-right). This is kind of like *chaining* the operations, applying one after another. From above, we know that something like `HEAD^^` is actually shorthand for `HEAD^1^1`, which in math notation you can think of as something like `(HEAD^1)^1`: you first take the first parent of `HEAD`, then take **its** first parent. An example of a mixture:

```
N^^~2  # The grandparent of the second parent of N
```

**Logical NOT**

You can also put the `^` in front, which is can be thought of as negation:

```
^N    # "not N"
```

This is a name for all commits that are *unreachable* fromm N.

**Logical AND**

You can combine such conditions:

```
^M N   # reachable from N but not from M
```

This pattern is so common that it has a shorthand:

```
M..N
```

We've seen this before with a linear history. In such cases, it's like the notion of a "range" of commits. This can be generalized to non-linear histories, where they technically mean "reachable from N but not from M".

**Logical XOR**

```
M...N
```

This means reachable from M or from N, but not both. This is useful in things like `git log`:

```shell
git log main...maint
```

This tells us the **"symmetric difference"** between the two branches: commits that are in `main` but not `maint`, commits that are in `maint` but not `main`, but not commits common to both.


## Submodules


**SCENARIOS:**

* Your project needs some other project's source code. For example, perhaps you want your system to be buildable on platforms even if another system is absent.
* There are some software packages that are intended be used only in source code form.


**CASE STUDY: The grep Source Code**


In `grep`, there's a submodule called `gnulib`. This is intended to be a **portability library** (source code only). It helps "emulate" Linux atop other operating systems like MacOS and Windows by implementing certain features that are absent on those systems.


### Usage


You can create a submodule from some source:

```shell
git submodule add https://github.com/a/b/c
```

This is similar to cloning, but instead of creating a new repository complete with its own `.git`, it:

* Creates an empty subdirectory.
* Create a `.gitmodules` file in the main project that establishes the relationship between the main project and the subdirectory.

Then, you run this, which initializes the submodule and then gets the source code from the provided link.

```
git submodule init
git submodule update
```


### Rationale


The other project(s) is evolving. This sort of setup gives you the ability to update to the latest version of the submodule at your convenience. You want a **controlled update**.

You can provide the commit ID to `git submodule update` to update to a specific version of the submodule based on the remote repository it's tracking.

Furthermore, updating a submodule is very simple. Instead of making edits to possibly to many files if you kept the dependency as part of your repository, the changes you make to the main project are simply which commit ID to use for the submodule. This keeps both your code and cognitive load modularized.

Also, you don't want to change the subproject directly. That's the responsibility of their maintainers. You as the consumer simply use what they have released to their repository, and if something is wrong with it, submit a bug report/pull request/etc.
# Version Control with Git: Porcelain Commands


## From the User's Viewpoint


This is usually from the perspective of a software developer/manager, and in some cases, the end user trying to use the software.

Three things are under Git's control:

0. Keep track of the working files (ordinary files), your source code, the "current state" of the repository.
1. Object database recording history of project development, where the project is modeled with a tree of files.
2. **Index**, recording your plans for the *future*.


## Getting Started


1. Create one remotely on a server (like GitHub) and clone it

  ```shell
  git clone URL
  ```

2. Create one locally and then link it to a remote):

  ```shell
  git init
  git remote add NAME URL
  ```

**Cloning** copies a repository AND creates corresponding working files.

Git sets up a special `.git` subdirectory, which is what makes the project directory a "repository".

**NOTE:** We don't necessarily have a "boss and servant" relationship between upstream and downstream repositories. Often times, a clone can become more active/popular than the original, in which case, the latter will start to sync with the former instead of vice versa.


## Exploring the Log


Display information about every commit leading up to the current version, in reverse order by default:

```shell
git log
```

We note that each commit has a unique ID, a long string after the word "commit", which is the **checksum** of the commit contents. A checksum is like a fixed bit integer that is a function of the bytes of the content it is encoding. This function must not produce **collisions**, where different contents produce the same checksum.


### Formatting the Log


```shell
git log --pretty=fuller
```

The fuller format shows that every commit has an **author** AND a **committer**, each with their own dates. Git distinguishes between these two contributors. This separation is routine for many larger projects. The author would be the person that writes the code, and the committer would be the overseer that reviews the and confirms the changes. These fields *establish responsibility* for changes, a major reason for using version control in the first place.

An interesting thing you may observe about repositories that have been developed for a long time is that you may notice commits with timestamps dating back to before Git was even around. This is because projects may have migrated from other version control systems, like RVS and CVS, and in copying over the history, the date data are all preserved.

<!-- From discussion notes. -->

Other examples:

```shell
git log --stat
git log --oneline
git log --pretty=format:"%h - %an, %ar: %s"
git log --oneline --decorate --graph --all

# Look for differences that change the occurrences of specified string
git log -S<string>
```


### Narrowing the Log


You can use the special `..` syntax to specify a commit range. The following displays the history between commits with ref `A` (exclusive) and `B` (inclusive), so like `(A, B]`. This is like "show me everything that led up to B, but exclude everything that led up to A":

```shell
git log A..B
```

Getting the *entire* history *up to* commit with ID `B`:

```shell
git log B
```

You can also use **"version arithmetic"** to get references to commits based on aliases, like branch names, tag names, `HEAD`, etc. [More on commit notation in Git Internals](Git%20Internals.md). The official documentation is also nice: https://git-scm.com/docs/gitrevisions.

> The special pointer `HEAD` by references the *current version* of the repository. This is kind of like a "you are here" pointer. You can move it around such as checking out to other branches, a specific commit, etc. in which case `HEAD` moves to the corresponding commit and Git changes your project directory to match the snapshot of that commit.

You can use the `^` syntax to specify the parent of a reference, so `HEAD^` means the commit just before `HEAD`, `HEAD^^` means the grandparent commit, etc.

Example about showing the most recent commit:

```shell
git log HEAD^..HEAD
git log HEAD^!  # shorthand
```


## Commit Messages


Commit messages are important because in essence, they help "market" your changes. They tell readers of the repository why certain commits were made and whether it was a "good" commit by explaining the *motivation* behind the changes. "Why are you making this change? Why shouldn't I just revert it?"

The rationale behind commit messages are similar to why you should comment your code. Oh yeah have I mentioned that:

> **COMMENTS SAVE LIVES. ALWAYS COMMENT YOUR CODE. PLEASE.**

There is overlap between comments in the source code and commit messages, but the primary distinction is the *audience*. Commit messages are more historically oriented, what you would tell the "software historian," people interested in the development of the repository as a whole. Comments in the source code are for the "current developer," people interested in having to study or change your code.

There are many style guidelines for commit messages out there, but here's Eggert's (which looks pretty standard in my experience).

Example commit message from the MIT repository shown in lecture:

```
Fix issues found by ASAN and Coverity

* tests/test_driver.pl: Preserve the LSAN_OPTIONS variable.
* tests/scripts/targets/ONESHELL: Don't set a local variable.
* tests/scripts/functions/let: Test empty let variable.
```

The first line should be at most 50 characters, and this acts as the "subject line" for the commit, like the elevator pitch. This should give any readers the *gist* of the commit.

The second line should be empty, separating the subject line from the body.

The remaining lines should be at most 50 lines, each at most 72 characters per line. Here you describe the finer details of the commit. You can use paragraphs, `*`-bulleted lists, etc.


## Working Files and Index


Technically a plumbing command, this is Git's version of `ls`, where it displays the current *working files*:

```shell
git ls-files
```

This helps us distinguish between general files in the directory and files that currently *matter* with respect to the repository. These are the files that are currently being **tracked** by Git.

```shell
rm $(git ls-files)  # lol!
```

Oh yeah, finding content within files is such a common pattern that `grep` is built into Git:

```shell
git grep waitpid
```

<!-- Yeah, um, the whole "plan for the future" way of putting it by Dr. Eggert didn't really make sense to me. -->

The **index** is what you have ready for the next commit, but have not committed yet.


## Commits and Staging


<!-- From discussion notes. -->

**Commits** are like checkpoints for your code, snapshots that are saved in the repository. Commits *append the index to the history*.

You know how it goes:

```shell
git add PATHSPEC
git commit -m MESSAGE
git push
```

There is an intermediate phase between modified/unmodified files and commit called the **staging area**. You can add files to this phase with `git add`

**Unstaging** a file:

```shell
git restore ---staged FILE
```

Syncing your local repository with the remote one:

```shell
git fetch
git pull  # effectively fetching + merging
```


## Viewing Status


<!-- From discussion notes. -->

Checking the status of your repository:

```shell
git status
```

Each file can be in the following states:

* Staged
* Not staged but modified
* Untracked


In general, files go through these states:

![](https://camo.githubusercontent.com/2f5084d2a28564283a8dff925fdf8fcf1e33377c1e879c33f94d2316f324654c/68747470733a2f2f6769742d73636d2e636f6d2f626f6f6b2f656e2f76322f696d616765732f6c6966656379636c652e706e67)


## Viewing Differences


`git diff` is similar to the GNU `diff` command, and like a more detailed version of `git status`.

> **HISTORICALLY:** The algorithm is very complex and was developed by a professor at the University of Arizona who went on to work on the Human Genome Project.

Viewing the difference between the *index* and the *working files*: `Δ(index vs working files)`:

```shell
git diff
```

This views the difference between the latest commit and the index: `Δ(latest commit vs index)`:

```shell
git diff --cached
git diff --staged  # equivalent
```

And this is `Δ(last commit vs working files)`

```shell
git diff HEAD
```

Compare the grandparent commit to the latest commit:

```shell
git diff HEAD^^..HEAD
```

<!-- From discussion notes. -->

More examples of viewing the difference between two commits:

```shell
# Typically with SHAs of the specific commits you want
git diff REF..REF

# But you can also abbreviate the hashes:
git diff 5c6cb30..53bf6bd
git diff 5c6c..54bf

# But this has a limit. This fails:
git diff 5c6..53b

# As usual you can use the HEAD ref to reference commits relative to
# the last commit:
git diff HEAD~..HEAD
git diff HEAD~4..HEAD
git diff HEAD^..HEAD
```


## Making Changes


Typical workflow:

1. Edit the working files.
2. Run `git add FILES...` to add the specified file contents to the index (the **staging area**, the **cache**). You can keep editing files and add any new changes to the staging area with the same command.
3. Run one of the `git diff` commands to verify that the changes are what you want.
4. Run `git commit`, which takes your index, makes a new commit, and puts it into the object database with the auto-generated checksum. In effect, it changes the commit `HEAD` references.

You're probably familiar with `git commit -m MESSAGE` that every Git crash course teaches you. This is useful for one-liners, but the default `git commit` actually drops you into your configured editor and allows you to write longer commit messages with the subject line and body format detailed above.

There is also `git commit -m MESSAGE FILE`, where `FILE` contains the extended message. This is useful for automating messages in scripting. Example:

```shell
git commit -m 'Fix issues from previous patch' README.git
```

Removing all **untracked files**:

```shell
git clean
```

This is useful for removing files created as part of some build process. If you're not sure, you can run a "what if" with the `-n` option:

```shell
git clean -n
```

> The `--dry-run`/`-n` switch is common to a lot of Git (and Unix in general) commands. It's a good way to "preview" the effects of a potentially destructive command instead of running it blindly right away.

There's also the `-x` option which cleans files that will even be ignored:

```shell
git clean -nx  # you best see what that would do first lol
git clean -x
```


## Configuring Git


### The .gitignore File


A special file inside the repository containing file patterns that Git should not track. The file pattern syntax is similar to the familiar **globbing pattern** as the shell.

.gitignore is like a configuration file that instructs how users run Git. It's under Git's control i.e. it'll show up in `git ls-files`.

**What files should be ignored?**

Files that we do not want to put under version control. Obvious candidates include:

* Temporary files, `\#*`
* Machine-dependent code, `*.o`
* Imported files (from other packages)
* Authentication information (passwords/keys/etc.)
* Hashes of passwords? If it's intended for authentication, this would be just as bad as raw passwords, so ignore them too. Hashes enable **rainbow attacks** on the passwords where attackers try to crack the checksum algorithm.


### The .git/config File


You can view the current configuration of the Git program with:

```shell
git config -l
```

This outputs the information stored in the editable `.git/config` file, which is specific to the current repository. Cloning a repository also copies the configuration file.

**CAUTION:** One notable problem (which is standard across any software) is that if there is a syntax error in the configuration file, Git stops working altogether.

`.git/config` is NOT under version control because it determines how Git itself functions and because it would introduce the problem of recursion. `.gitignore` IS under version control because it's like a message from the developer and contains information about how to manage the project actually being version controlled. You also don't need to worry about what's in `.gitignore` to use Git itself.


### The ~/.gitconfig File


After resolving the configuration in the current repository, Git then falls back to this configuration file. Contains *global* configuration information for Git, like username and email.

<!-- From discussion notes. -->

You can edit this file manually with your editor of choice, but you can also use `git config` to write directly from the command line:

```shell
git config --global KEY VALUE
```

*Setting up Git on New Machines:**

```shell
git config --global user.name "Vincent Lin"
git config --global user.email vinlin24@outlook.com
```

The user name is not actually that important. It's mostly used for identifying contributors at a glance with things like `git log` I assume. The email however is *critical* because remote services like GitHub use that to identify the account of the contributor.

<!-- Added because I thought it was cool. -->

Other cool things you can specify in this file:

```ini
[core]
    # If you don't like being dropped into Vim by default,
    # This sets it to VS Code (you can also use Emacs, etc.)
    editor = code

[alias]
    # Abbreviations
    s = status
    co = checkout
    cm = commit -m
    # etc.
    # I have much more ehe
```


## Show a Commit


`git show` is a generic command that shows a commit "object". Commit objects live in the database and are really just the recorded changes from the previous commit along with some metadata.

The ubiquitous `--pretty` option can be used here too for more verbose output:

```shell
git show --pretty=fuller
```


## Working with Remotes


A **remote**, named `origin` by convention, is the **upstream** repository from which the local repository was cloned or set to track.

> The concept of **upstream/downstream** comes from the fact that clones may be sourced from repositories that are themselves sourced from another branch, forming a chain of origin - a "stream".


### Fetching


This consults the remote server for upstream changes and syncs the clone's "opinion" of what upstream looks like:

```shell
git fetch
```

If it outputs nothing, it means the local clone is up-to-date with the upstream remote. `fetch` does not change the working files nor does it alter any branches, only your *local* copies of the *remote* branches, like `origin/main` instead of `main`.

`fetch` is incremental, only fetching the changes since the last call to fetch.


### Pulling


`git pull` is roughly equivalent to a `git fetch` followed by `git merge` of those upstream changes into the current branch. This command is actually rarely ideal on large development projects because more often than not it may not be what you want to do. When it works, `pull` DOES change the working files.

A good habit to get into is to first download and preview the upstream changes with `git fetch`, and then when you're ready to integrate those changes, run `git pull` or merge them manually.


### Fetching vs. Pulling


<!-- From one of my Piazza follow-ups. -->

The commit objects are downloaded from upstream and made available on your local repository, you just don't immediately notice them because they update your copies of the **remote** branches; `git fetch` is a benign command because it leaves your **local** branches untouched. Suppose you had a simple history on your local machine. If you have a remote repository set up, with your `main` set to **track** the remote's version aka `origin/main`, your local repository would look like:

```
A <- B <- C (main, origin/main)
```

But maybe in reality the upstream repository (e.g. on GitHub) has updated since then:

```
A <- B <- C <- D <- E (origin/main)
```

Git can't know this *until* you run `fetch`, because that's when it communicates with the remote server. When you run `git fetch origin main`, your local repository does two things:

1. Download the new commit objects, `D` and `E`.
2. Update your branch tip representing the remote branch i.e. `origin/main`.

```
        (main)
          v
A <- B <- C <- D <- E (origin/main)
```

Notice your `main` didn't move - it doesn't even know what happened. You as the developer can see what was really brought in by specifying `origin/main` at the command line, like with:

```shell
git log main..origin/main
```

When you're ready to update your **local** `main` with these new changes, *then* you can run something like:

```shell
git pull origin main
```

But as Victor pointed out in @659_f1, `git pull` really is just `git fetch` followed by `git merge`, so since you already did the `fetch` part, you can also just complete the merge:

```shell
git merge origin/main
```

**Now** your local repository is updated to match the state of your upstream:

```
A <- B <- C <- D <- E (main, origin/main)
```


## Recovering from Mistakes


### Before the Commit


The working file is wrong, but the most recent version saved in the repository is safe. You haven't committed your erroneous changes yet, so simply:

1. Edit the file `F`.
2. Update the index: `git add F`.


### After the Commit


The repository now has a bad version of the code in its history.

**Option A:** Commit a new, fixed version. The bad commit would still be recorded in the the old and fixed commit. This would be an honest representation of the development history, but often times this is not wanted.

```
(old)-->(broken)-->(fixed)
                      ^
                     HEAD
```

> An iron-clad rule in Git is that *you cannot change history*. This is because every commit is uniquely identified by the SHA-1 ID.

**Option B:** However, you can cheat this rule with the `git commit --amend` approach, which creates a new child from the parent commit and moves `HEAD` to it:

```
(old)-->(broken) # effectively discarded
  |
  +---->(fixed)
           ^
          HEAD
```

This is very risky as an upstream repository. If someone happens to `fetch` when `HEAD` still points to the broken commit, then when they `fetch` with the new altered tree, Git and the users will get confused.

**Option C:** You can change the state of the repository back to another version.

Reverting to the previous commit:

```shell
git reset HEAD^
```

This would fail if there are changes in the working files, in which case, you can throw the changes away and revert anyway with:

```shell
git reset --hard HEAD^
```


## Branching


A single commit can have multiple children.

A **branch** in Git is like a lightweight, movable *name* for a commit that is the at the tip of a line of maintenance.

> At the end of the day, Git is all about pointers! :D

One branch, the "main"/"master" branch is typically reserved for *mainline development*. There may be other branches for things like *maintenance development*, *old releases*, *hot fixes*, etc.


### Patching Across Branches


Suppose a security hole was discovered in an old commit, which multiple branches share as an ancestor. You can fix the bug on the mainline branch, but that doesn't solve it for other branches.

The solution is to **cherry-pick fixes**. You manually apply the same Δ to all versions that have the same bug.

Suppose there's an alternate branch named `maint`.

```shell
# Your familiar sequence
git add F
git commit -m "Make an emergency fix"

# Prepare the patch to apply to other branches
git diff HEAD^! > t.diff

# t.diff is a working file, preserved across checkout
git checkout maint

# Apply patch to this branch's working files
patch < t.diff
git add F
git commit -m "Make an emergency fix"
```

The `patch` command is actually external to `diff`. It reads the output of the diff file and modifies the old file so that it looks like the new file:

```shell
diff -u A B > AvsB.diff
patch < AvsB.diff
```

This modifies `A` to look like `B`.

Attempting to apply a patch to a since edited version of a file may fail to work. It may still work if the changes to the original files does not *collide* with what the patch is attempting to change.

`diff` operates on **hunks**, batches of lines that represent a change. Patching goes through each hunk and applies the change. If the hunks do not match, then it will reject the change into an `rej` file, prompting you to fix it by hand.

**NOTE:** The output of `diff` is NOT deterministic. There is no requirement of the algorithm to modify a file in a specific way as long as the final copy is correct.

<!-- From discussion notes. Honestly I don't know what these do. -->

**Patching and Cherry-Picking**

Create a patch file(s) for a commit

```shell
git format-patch [-o DIR] REF
```

```shell
git am FILES...
git apply FILES...
git cherry-pick REF
```


### Manipulating Branches


Listing all branches, optionally verbosely:

```shell
git branch -a [-v]
```

Switching between branches:

```shell
git checkout REF
```

Creating a branch off a commit, defaulting to `HEAD`, and checking out to it:

```
git checkout -b NAME [REF=HEAD]
```

**EXAMPLE:** creating and checking out to a new branch named `newbr` off of the grandparent of the current `HEAD`:

```shell
git checkout -b newbr HEAD^^
```

Branch names must be unique. Git won't let you create or rename a branch to an existing name. As you know from HW6, this is because information about branches are stored as physical files in the file system under `.git`.

Because branches are just names, *deleting* a branch does not modify any commits, just a reference that used to point to one.

```
git branch [--delete | -d] NAME
```

You can try to do something weird like:

```shell
git branch -d master
```

The objects would still be there, but Git will lose track of where they are, so Git would warns you. You can *forcefully* delete a branch despite Git's warnings with:

```
git branch -D NAME
```

*Renaming* a specific branch, defaulting to the current branch:

```
git branch -m NEW_NAME [OLD_NAME=HEAD]
```


### Detached HEAD State


You can checkout to an arbitrary commit by ID/tag name:

```
git checkout REF
```

But this puts you in **detached HEAD state**, which is when `HEAD` is not pointing to any branch tip. Git warns you that you can look around but not make further changes. You cannot commit in this state because Git does not know how.

However, if you want to make changes from this version of the codebase, you can checkout to a new branch off this commit as you normally would:

```shell
git checkout -b mybranch
```


## (1) Merging


**Merging** is one way to bring together multiple lines of development. You integrate changes **from** one branch **to** another branch. More technically, it is when you create a commit from two or more parents commits, which may or may not be named branch tips.


### Mechanism


Suppose:

```
                      this is the merge commit
                              v
()<--(A)<--()<--()<--(Y)<--(Merged)<-- ...
      |                       |
      +------()<----(X)<------+
```

Git finds the common ancestor `A`, of the parent commits `X` and `Y`. Then, it runs computation on all 3. It is as if Git runs:

```shell
diff3 X A Y > combined.diff  # "3-way diff"
```

This file describes changes to change the common ancestor `A` to *either* `X` or `Y`. Git then applies those changes and creates a *new* commit instance for it, the **merge commit**.

The command to merge a branch named `BRANCH_NAME` into the current branch:

```
git merge BRANCH_NAME
```

What this does is:

1. Compute 3-way merges.
2. Replace working files accordingly.



### Merge Conflicts


More often than not, the changes *collide*, resulting in a **merge conflict**. If there's a collision, Git modifies the affected files with a "replica" of the collision with the special notation. Without a GUI, conflicts in their raw form actually look like:

<!-- Editors like VS Code are smart enough to recognize this as a merge conflict, so if you're viewing this source code, you'll see the funky widget you usually use to solve conflicts. -->

```
<<<<<<<
A (Current Change)
=======
B (Incoming Change)
>>>>>>>
```

<!-- From discussion notes. -->

Git actually modifies the content of the conflicting file with this pattern, conflicting text separated by special barriers. The one with `<` brackets shows the **CURRENT** content, and the one with `>` brackets shows the **INCOMING** content. To resolve the conflict, you need to edit this block to only include one version of this content ("accept current change" or "accept incoming change"). You can also accept both changes. You could also leave the file in this conflicted state with the barriers, but that's stupid practice because if you do this on a source file, it will almost definitely be a syntax error.

**Resolving conflicts:** usually you would edit the conflicting files in your editor of choice and regularly check for further instructions with `git status`. You can just conclude the merge with:

```shell
git merge --continue  # but not recommended
```


## (2) Rebasing


Alternatively, one can **rebase** a commit onto another branch. This takes away the problem where reviewers have to worry about common ancestry and a bunch of diffs. They only need to examine a linear history.

```
( )<--(common)<--( )<--( )<--(main)
          |
          +--( )<--(my)
          Δ1     Δ2
```

Often times, the main branch undergoes complex changes since the branch last diverged, so it may be easier to just move the commits of the merging branch right to the tip of the branch it's merging into.

```
( )<--(common)<--( )<--( )<--(main)
                               |
                               +--( )<--(my)
                               Δ'1    Δ'2
```

Git does this by taking the deltas Δ1 and Δ2 and then combining them with changes at `main` since `common` to compute new deltas Δ'1 and Δ'2. From the user's perspective, it's like we plucked off the `my` branch and attached it to the tip of `main`.


### Merging vs. Rebasing Pros/Cons


<!-- Table from one of my Piazza answers. -->

| Merging                                                                                                               | Rebasing                                                                                                                                                                                                                                                        |
| --------------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| ➕ Only one commit is created per merge.                                                                               | ➖ A new commit is made for every commit you rebase.                                                                                                                                                                                                             |
| ➕ Does not change existing commit history, so you're less likely to screw over others working on the same branch.     | ➖ Changes existing commit history. If you misuse this command, you could mess up an important branch like `main` for everyone else. See [the Golden Rule of Rebasing](https://www.atlassian.com/git/tutorials/merging-vs-rebasing#the-golden-rule-of-rebasing). |
| ➕ Your steps can be fully retraced because you know when each merge was performed.                                    | ➖ It is difficult to see when a rebase actually occurred.                                                                                                                                                                                                       |
| ➖ If you have to merge often, these merge commits may pollute your history and make it harder to understand.          | ➕ No unnecessary merge commits polluting your history, making it easier to understand.                                                                                                                                                                          |
| ➖ Your history will still have interweaving branches that may make navigation (`git log`, `git bisect`, etc.) harder. | ➕ Keeps your history as linear as possible, making it easier to navigate with such commands.                                                                                                                                                                    |

Another downside is that you may repeatedly rebase if progress resumes on the `main` branch while review was still pending.

**IMPORTANT:** The fact that rebasing manually linearizes history is a point of philosophical concern. Some may see this as an upside because it makes it easier to review, but others may see rewriting history as a bad thing.

> **ASIDE:** Often times, project managers use Git history to see which programmers are better and who deserve bonuses. This is not always the best approach. The history does not tell you the full story. There is a very tenuous relation between number of commits and the value of a programmer to a project. A glaring example at this time of writing is the mass layoffs at Twitter where *genius* Elon Musk thought to fire people who have written the least lines of code.

> The *essence* of Git and any version control system is that you are editing *changes* to source code, not the source code itself. Understanding this is what sets a **software developer** apart from an ordinary "programmer".

> If you find yourself in an environment where people are lying on purpose about commits in order to improve their job prospects or something, you're in the wrong company. **- Dr. Eggert**


## (3) Stashing


Rebasing is less "formal" than merging. Changes are looser, but are recorded as a sequence of commit nonetheless. Stashing is even less formal than rebasing. Changes are only floating around in your repository, waiting to be reapplied.

The scenario looks something like:

1. You're working on the next change in your branch.
2. You want to switch to some other branch NOW. You *could* commit what you have right now, but that is bad practice because you're essentially committing junk. "You want your repository to be in good shape at all times."
3. Instead, you could save your changes in an external file:

   ```shell
   git diff > mywork.diff
   git checkout -f  # Discard working files
   ```

4. Checkout to the other branch and do work on it

  ```shell
  git checkout main
  ```

5. Checkout back your original branch and patch it.

  ```shell
  patch < mywork.diff -r1
  ```

Git actually provides a way to do this within Git itself, using the `stash` command. At step 3, you would do something like:

```shell
git stash push
```

This saves the state of your working files in some part of the index. When you want to retrieve this state, you can get it from the stash stack with:

```shell
git stash apply
```


## Bisecting


Suppose you have a linear piece of history where somewhere between a stable version and the most recent commit, something went wrong. You can think of this problem of finding the first faulty commit as partitioning the timeline into OK and NG ("not good") sections, hence *bisecting*.

The timeline is "sorted" in that if you think of OK=0 and NG=1, the history will always be such that all NGs follow OKs.

```
                    |
(v4.3)<--( )<--( )<---( )<--( )<--(main)
OK        OK    OK  |  NG    NG     NG
                    |
```

This then becomes a classic *binary search* problem, where we can identify the first NG commit in O(logN) time.

Starting a bisect in Git:

```shell
#                 NG   OK
git bisect start HEAD v4.3
```

Then we tell Git to run your check script on each commit and use the exit status to determine if the commit is OK or NG:

```shell
#              vvvvvvvvvv any shell command
git bisect run make check
```

In this case, we use a Makefile with a `check` target that defines some test cases for the program

Of course, this also introduces the problem that if your test cases are buggy, then you may get false alarms. If you know ahead of time that a commit, say `v3`, will produce unreliable test results, you can skip it with:

```shell
git bisect skip v3
```


## Collaborative Best Practices


<!-- From discussion notes. -->

**Branches:**

* `master` or `main`: stable branch
* `develop`: for development
* Each team member may create their branches for individual features/bugs

Protect the `master` branch. DON'T force-push; it could destroy the commit history. Don't be that guy.

On GitHub, use **pull requests** to merge changes into other branches. Pull requests may undergo a **review**. You can also use **issues** to assign bugs or features to team members.

Avoid merging temporary files or code meant for debugging (like dummy data).
# The Internet


## Client-Server Computing


The Client-Server model, where many browser instances can interact with a single web server, which in turn may be connected to some database:

```
C                     S
+---------+         +------------+
| browser | <-----> | web server |
+---------+         +------+-----+
C                    ^     |
+---------+          |  +--+--+
| browser | <--------+  | DB  |
+---------+             +-----+
```


### Alternatives to the Client-Server Model


0. **Single computer** (CS 31 assignments, where the logistics of networking, OS, etc. were abstracted away).

1. **Peer-to-peer (P2P)**: Decentralized approach. If a peer doesn't have a resource, the request is redirected to another resource. The main advantage of this is that it is more fault-tolerant: a single peer going down doesn't bring the system down. The downside is that it is more involved to maintain a consistent state across every peer. This is in contrast to the less fault-tolerant but more state-consistent client-server model.

2. **Primary secondary**: One primary machine that serves as the "overseer" - it keeps track of how the application is split up among numerous secondary servers. The secondary servers receive a small "subproblem" of the application from the primary server and return any results.


### Performance and Correctness Issues


Traditional programming performance metrics:

* CPU time: how many CPU instructions executed (roughly proportional to the amount of energy consumed)
* Real time: amount of human time elapsed
* RAM
* I/O

Networking performance metrics:

* **Throughput**: Number of client requests per second that the system can handle (assuming individual requests and responses are reasonably small and approximately equally sized); bigger is better.
* **Latency**: Delay between a request to the server and the response back from the server; smaller is better.

To improve **throughput**:

* ➕ You can perform actions "out of order" (compared to "request order").
* ➕ You can perform actions *in parallel*.
* ➖ This can cause out-of-order execution, which can "mis-order" transactions.

To improve **latency**:

* ➕ You can use **caching** to speed up responses.
* ➖ This can cause **stale caches**, which requires **cache validation** to fix, which could be an expensive operation.


## Task Networking Styles


**Circuit Switching**

* System is connected to the nearest central office, which can connect to other central offices. In the end, you get a path between the one computer to the other.
* However, you have temporary ownership of a wire during the transaction.

**Packet Switching**

* Connected to a little computer that breaks the signal into a bunch of small messages called **packets**. Each packet is sent to a local **router** that sends the packets along the network to the destination. Each packet travels independently and possible along different paths, and they do so very quickly, so it does not back up the network


## Packet Switching


Idea originally proposed by Paul Baran (1960s, RAND).

Sold to Department of Defense as a reliable way to communicate in the event of nuclear war because of its ability to *reroute* in the event of an office goes down, which gives it an edge over traditional **circuit switching**.

Packet switching is a best effort transmission/no guarantees (circuit-switching is a *guarantee* between the two machines).

**Problems:**

* Packets can be lost - if some router along the way gets overloaded, some packets may be discarded
* Packets can be received out of order
* Packets can be duplicated via a mechanism called **bridges**
* All routing decisions are local to the individual routers, so packets may be directed into a loop


## Internet Protocol Suite (TCP/IP)


A **protocol** is simply a set of rules. This particular set of rules forms the foundation of the Internet.

Basic idea: **layering**. Protocols are built on top of each other, with each layer abstracting and extending the one below it:

```
                          End user
                            |
+-----------------------+   |
|   Application Layer   | Applications
+-----------------------+   |
|    Transport Layer    | Data channels
+-----------------------+   |
|    Internet Layer     | Packets
+-----------------------+   |
|      Link Layer       | Bits and bytes
+-----------------------+   |
                            |
                          Hardware
```


## Link Layer


Point-to-point.

The bits and bytes you send on a single link from one node to another (no routers in between). Very hardware oriented. Each technology used has its own link layer protocol.


## Internet Layer


**Packets.** Software almost never operate directly at this level because it is too low level.

The IP (**internet protocol**) comes in different versions:


### IPv4


Created in 1983, specified by Jon Pastel (UCLA), etc. Involves packets, connection.

We are running out of IPv4 addresses because there are only about 4 billion, and the US has most of them.

**Packet Anatomy**

A packet is just a sequence of bytes. The **payload** is prefixed by a **header** that stores **metadata** like:

* Length.
* Protocol number (to support any protocol that ends up being built atop IP) tells you the *type* of packet that's being transported so algorithms can determine their priority. For example, a single video frame is much less important than part of a image file.
* Source IP address (a 32-bit number often expressed in Base 256).
* Destination IP address (ibid).
* A **checksum** (16-bit).
* **A time-to-live (TTL)** field (8-bit "hop count" that keeps track of how many routers it goes through; packets with abnormally high TTL values get dropped to prevent packet loops).

When routers receive packets, they look at the header, especially the destination address, to determine what to do with it.


### IPv6


* Created in 1996.
* 128 bit IP addresses.
* The headers are also longer.
* A superset of IPv4; the 32 bits of IPv4 can be mapped to 32 bits of an IPv6 address, so IPv4 users can communicate with IPv6 users. The converse is not as simple but made possible with complicated translation techniques.
* Less efficient because the extra length is overhead for the link layer


### UDP (User Datagram Protocol)


* Created by David Reed at MIT.
* Designed as a thin layer over IP, but still at the Internet layer.
* You use UDP if your application sends single short messages over the Internet without much care if it is lost, duplicated, etc. Intended for apps that *want* to deal with packets.


## Transport Layer


**Data channels.** Large data *streams* (TB of data) that cannot be individual packets. This layer oversees how a *stream of packets* is transmitted.


### TCP (Transmission Control Protocol)


Vint Cerf from Stanford, Bob Kahn from Princeton.

Looks like a *stream of data* that:

* Is reliable (via **acknowledgements**).
* Have **sequence numbers** for packets.
* Is ordered (the recipient reassembles the packets that may be out of order in the lower layers).
* Is **end-to-end error-checked**.

The protocol has:

* Flow control; sends packets at the correct rate to not overload the network.
* Retransmission.
* Reassembly.

A single machine can listen to multiple TCP channels on different **ports**.


## Application Layer


Application dependent: web, voice, etc.


### RTP (Realtime Transmission Protocol)

Runs atop UDP because TCP is not suited for sending realtime data. TCP would cause **jitter**.


### HTTP (HyperText Transfer Protocol)


Runs atop TCP because reliability is a must - a single misplaced bracket may break an HTML document.

Tim Berners-Lee at CERN in 1991. Honestly a real chad, he invented:

1. The Web, HTTP and its **request-response protocol**:

   * Create a TCP connection (default port 80)
   * Client sends the server a GET message
   * Server responds with the contents of the webpage

2. **HTML (HyperText Markup Language)**: a way to express the contents of a webpage in a machine-independent format.

> The Internet fundamentally is just HTML and HTTP combined together. HTML is like the content of the Internet, HTTP is how it gets around.


### `telnet` Aside


You can use the `telnet` command to open a 2-way stream where you can send raw HTTP requests. Run `telnet` with the IP address and the port to connect to. THen enter the `GET` command with the resource you want to get and the protocol version to use. Then specify the `Host`:

```console
$ host www.cs.ucla.edu
WWW.cs.ucla.edu has address 164.67.100.182
$ telnet 164.67.100.182 80
Trying 164.67.100.182...
Connected to 164.67.100.182.
Escape character is '^]'.
GET / HTTP/1.1
Host: www.cs.ucla.edu

HTTP/1.1 301 Moved Permanently
Date: Wed, 19 Oct 2022 22:21:10 GMT
Server: Apache
X-Frame-Options: SAMEORIGIN
Location: https://www.cs.ucla.edu/
Content-Length: 232
Content-Type: text/html; charset=iso-8859-1

<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML 2.0//EN">
<html><head>
<title>301 Moved Permanently</title>
</head><body>
<h1>Moved Permanently</h1>
<p>The document has moved <a href="https://www.cs.ucla.edu/">here</a>.</p>
</body></html>
```

The actual HTML data is prefixed with a response **header**, detailing metadata like the response code, content length, etc.


### Variants of HTTP


**HTTPS (HTTP Secure)**

With plain HTTP, every router between the source and destination can see the raw data that is transmitted.

Uses shared private keys or something.

**HTTP/2**

Came out in 2015.

Added extra features to HTTP:
1. Header compression
2. Server push (lets the server send a response without a request)
3. Pipelining (allows client to send multiple requests so the server can respond in batches, allows more *parallel* communication instead of having the client wait for a response every single time)
4. Multiplexing (talk to multiple websites over one TCP channel)

**HTTP/3**

Not released yet.

1. Now based on UDP instead of TCP (motivated by the increase in voice/video services)
2. Uses even more multiplexing
3. Avoids jitter by avoiding **head-of-line** blocking delays; allows content after a dropped packet to be delivered (?)


## Data Languages


HTML is an example of a data language. We also have:


### SGML (Standard General Markup Language)


Markup language for documents (1980s).

A **declarative** (as opposed to an **imperative**) language. **Markup** specifies the structure and attributes of a document.

<!-- HTML for highlighting, SGML doesn't seem to be supported -->
```html
<QUOTE TYPE="example">
    OK <ITALICS>This text</ITALICS> is
    part of a block quote intended for example.
</QUOTE>
```

Extensible bracket types allow you to define new features without making breaking changes to the language. `<QUOTE>` is one type of bracket with its closing tag `</QUOTE>`, `<ITALIC>` is another, etc.

The structure of the document forms a tree structure: the content inside `<ITALICS>` is a child node to `<QUOTE>`.
# The grep, sed, and awk Commands


## The `grep` Command and Regular Expressions


Normal invocation syntax:

```shell
grep PATTERN FILE1, ..., FILEn
```

If it finds a line with the PATTERN, it outputs it. So it is similar to `cat`, but more selective. In fact, if you use a pattern that matches every character, `grep` can behave identically to `cat`:

```shell
grep '.*' file
```


### CHEATSHEET: Partial of `man grep`


```
NAME
       grep, egrep, fgrep, rgrep - print lines that match patterns

SYNOPSIS
       grep [OPTION...] PATTERNS [FILE...]
       grep [OPTION...] -e PATTERNS ... [FILE...]
       grep [OPTION...] -f PATTERN_FILE ... [FILE...]

DESCRIPTION
       grep  searches  for  PATTERNS  in  each  FILE.  PATTERNS is one or more
       patterns separated by newline characters, and  grep  prints  each  line
       that  matches a pattern.  Typically PATTERNS should be quoted when grep
       is used in a shell command.

       A FILE of “-”  stands  for  standard  input.   If  no  FILE  is  given,
       recursive  searches  examine  the  working  directory, and nonrecursive
       searches read standard input.

       In addition, the variant programs egrep, fgrep and rgrep are  the  same
       as  grep -E,  grep -F,  and  grep -r, respectively.  These variants are
       deprecated, but are provided for backward compatibility.

OPTIONS
   Pattern Syntax
       -E, --extended-regexp
              Interpret PATTERNS as extended regular  expressions  (EREs,  see
              below).

   Matching Control
       -e PATTERNS, --regexp=PATTERNS
              Use  PATTERNS  as the patterns.  If this option is used multiple
              times or is combined with the -f (--file) option, search for all
              patterns  given.   This  option can be used to protect a pattern
              beginning with “-”.

       -f FILE, --file=FILE
              Obtain patterns from FILE, one per line.  If this option is used
              multiple  times  or  is  combined with the -e (--regexp) option,
              search for all patterns given.  The  empty  file  contains  zero
              patterns, and therefore matches nothing.
```


### Basic Regular Expressions


`grep` actually uses **Basic regular expressions (BREs)**, a simpler form of the more familiar regex, **extended regular expressions (EREs)**. It also only matches against single lines at a time.

* Most printable characters like letters and numbers match themselves.
* Control characters like `*` need to be escaped in order to match themselves e.g. `\*` and `\\`.
* `.` as a control character matches every single character
* `^` matches the start of the line
* `$` matches the end of the line
* `[]` matches a single character but only those that are included in the set enclosed in the square brackets

**CAUTION**: the `*` is a **globbing pattern** to the shell, so it's interpreted with special meaning. Enclose it with *quoting* - don't let the *arguments themselves* be interpreted by the shell.

```shell
grep 'ab*c*' fo
```


### Remember to Quote!


Quoting and little language commands like `grep` go hand-in-hand. Scripts are evaluated by the shell first before being shipped off as arguments to its child programs, so make sure you quote and/or escape everything keeping in mind how your initial string will be **resolved** as it makes its way through each program.

As described in [single quoting](Shell.md#single-quoting), single quotes are often used for `grep` expressions because they preserve everything literally - what you see within the quotes is *exactly* what you're giving `grep`.

When your expression gets more complicated however, you will likely have to use [double quotes to be able to interpolate](Shell.md#double-quoting) regex fragments into the larger expression. In those cases, you'll likely see many instances of double backslashes `\\` since `\` is still a special character within double quotes which much be **escaped** to represent themselves by the time it reaches `grep`.

An example from a practice midterm:

```shell
atom='[a-zA-Z0-9]+'         # at least 1 alphanumeric character
string='\\"([^"\]|\\.)*\\"' # \"(something)\", where (something) is . OR neither " or \
word="($atom|$string)"
words="$word(\\.$word)*"
grep -E "$words" | grep ' '
```


### ASIDE: grep Edge Case


Example:

```shell
grep '*' foo
```

This is a weird case where `*` actually matches itself instead of being treated like a quantifier. grep has some ambiguous edge cases.

But *sometimes* grep yells at you:

```console
$ grep '['
grep: Invalid regular expression
$ grep '\['
```

So basically, just use something well-defined.


### Extended Regular Expressions


Historically there was another team that came up with alternate syntax to the familiar grep, called egrep. Nowadays you can use the `-E` flag to specify that the pattern is using the extended syntax instead:

```shell
grep -E PATTERN FILE1, ..., FILEn
```

It provides some features in addition to the BREs:

* The `+` quantifier to match "1 or more occurrences," so `P+` is equivalent to `PP*`.
* The range quantifier: `P{2,5}`. This would match the pattern `P` 2 to 5 times in a row, inclusive.
* Grouping with `()` and combining patterns with `|` to mean logical OR. For example, `(hello|foo)` matches *either* the entire string `hello` or the entire string `foo`.

Note that these features means the characters `+` `{` `}` `(` `|` `)` become **meta-characters** in EREs. In BREs, they would match themselves literally.

**NOTE:** `{}` is *also* a **globbing pattern** to the shell, where `{a,b,c,...}` expands to `a b c ...`, so remember to [quote your regex](#meta-character-dangers)!

Oh yeah and *apparently* in EREs, you can use the OR `|` operator outside of a group `()`:

```
hello|there
```

<!-- From midterm review. -->

**EXAMPLE: Write an ERE that only matches numbers between 0 and 255.**

```
^(25[0-5]|2[0-4][0-9]|1[0-9]{2}|[1-9][0-9]|[0-9])$
```

**TIP:** Make sure to remember to include `^$`! That constrains the number of digits you can match, and more importantly, it ensures no partial matching.


### Both BREs and EREs


* Bracket expressions `[]` are available in both standards and specify a **character set**.
* **Ranges** in bracket expressions like `[a-z]`. The `^` at the *start* negates the set e.g. `[^a-z]`. To include the literal `^` inside the character set, put it at the end.

Within a set, these characters take on special meanings, so to match them literally:

* To match the `]`, put it at the start of the set like `[]abc]`
* To match the `^`, put it at the end of the set like `[abc^]`
* To match the `* , put it at the end of the set like `[abc^* `

There are also **named character sets** in `grep`:

```
[[:alpha:]]     # match every alphabetic character
[[:alpha]$/]    # match every alphabetic character or $, /
[[:alpha]$/\]   # backslash isn't special inside a bracket
```

Common use cases of regular expressions in things like web applications include:

* Validating phone numbers
* Validating emails
* Extracting or processing such information


## The `sed` Command


A command that is a like a generalization of `grep`, `head`, `tail`, etc.

Short for "stream editor". `sed` was designed to let you edit a file of ANY size because it does not use a buffer; rather it uses a **stream**. It's a "programmable incremental editor".

**Using sed to emulate previous commands:**

```shell
sed -n '1,10p'      # head -n 10
sed -n '10q'        # head -n 10
sed -n '$p'         # tail -n 1; can't generally because sed is incremental
```

`sed` itself is a little language that has supports its own scripting The invocation pattern is:

```
sed OPTIONS... [SCRIPT] [INPUTFILE...]
```


### Stream Manipulation


The SCRIPT can be simple like:

```shell
sed 2q input.txt
sed 2,4d input.txt
```

* `q` example: **q**uit after the `2`nd line.
* `d` example: **d**elete lines `2` through `4` (basically excluding it from the output).


### String Substitution


More commonly, the SCRIPT has the general structure below. By DEFAULT, *sed works at the line level*, replacing the FIRST occurrence of PATTERN with REPLACEMENT *per line*.

```
'[RANGE] s/PATTERN/REPLACEMENT/FLAGS'
```

* Optional RANGE narrows the operation to a range of lines, like `1,3`.
* By default, PATTERN is interpreted as BRE, but can be changed to ERE with the `-E` flag.
* If you used capturing groups in PATTERN, you can retrieve them in REPLACEMENT with `\1`, `\2`, etc.
* FLAGS are zero or more characters, each representing an individual modification. Some examples are:

| Flag | Meaning                                                           |
| ---- | ----------------------------------------------------------------- |
| g    | **g**lobal: replace all occurrences per line instead of the first |
| NUM  | replace only the NUM-th (e.g. 1st, 2nd, etc.) occurrence per line |
| p    | if the substitution was made, then print the new pattern space.   |

This is an example from lecture where you remove trailing whitespace (not sure if I copied it down correctly):

```shell
sed 's/[[:space:]]*$//; /^$/d'
```


### Tutorial Examples


Dummy data in `toppings.txt` from https://www.youtube.com/watch?v=nXLnx8ncZyE:w

```
Pizza topping combos:
1. Spinach, Pepperoni, Pineapple
2. Pepperoni, Pineapple, Mushrooms
3. Bacon, Banana Peppers, Pineapple
4. Cheese, Pineapple
```

Replace the occurrences of `Pineapple` with `Feta` in the buffer, and then output the result.

```shell
sed 's/Pineapple/Feta/' toppings.txt
```

To modify the file **in-place**, just use the `-i` flag:

```shell
sed -i 's/Pineapple/Feta/' toppings.txt
```

Deleting every occurrence of a string is as simple as using the empty string for REPLACEMENT:

```shell
sed 's/Feta//' toppings.txt
```


### Delimiters and Escaping


The `/` is just standard practice for the **delimiter**, which can be changed. The delimiter is automatically picked up from the character directly after `s`:

```
s|PATTERN|REPLACEMENT|
s PATTERN REPLACEMENT
s.PATTERN.REPLACEMENT.
```

This comes in handy when you would rather not have to escape characters, like the `/` itself. If you insist, you can use `\` to escape characters, like `\/`.


## The `awk` Command


In a sense, even more general then `sed`. A scripting language designed to edit text. It's a programming language with variables, arrays, regular expressions, etc.

Here are some examples from lecture:

```shell
awk '{ x = $0; print "("x")"; }'
awk '{ if ($0 == x) print $0; x = $0 }'  # outputs duplicates
```


### Basic Usage


Invocation pattern:

```
awk [OPTIONS] SCRIPT INPUTFILE
```

The SCRIPT has the general structure:

```
'{command args}'
```

*awk also works at the line level.* The SCRIPT body is assumed to operate individually on *each* line of the input. By default, awk sees each line as a sequence of **fields** delimited by spaces:

```
foo bar baz spam eggs
$1  $2  $3  $4   $5
```

### Printing Select Fields


Dummy data `tmnt.txt` from https://www.youtube.com/watch?v=oPEnvuj9QrI, a file with 4 lines and *3 fields each*:

```
leonardo blue leader
raphael red hothead
michelangelo orange party-animal
donatello purple geek
```

The `print` command prints the content of each line, so alone it is identical to `cat`:

```shell
awk '{print}' tmnt.txt
```

The `$NUM` syntax selects the NUM-th (starting from 1) field from each line. Paralleling regex capture group 0, `$0` selects the entire line.

```console
$ awk '{print $1}' tmnt.txt
leonardo
raphael
michelangelo
donatello
```

You can select multiple fields by delimiting them with commas. The fields will be outputted with spaces in between them:

```console
$ awk '{print $1,$3}' tmnt.txt
leonardo leader
raphael hothead
michelangelo party-animal
donatello geek
```

An example of pipeline input, outputting the permission flags next to the file names within the current directory (inspecting `ls -l`, the permissions and names happen to be on columns 1 and 9 respectively):

```console
$ ls -l | awk '{print $1 $9}'
-rw-r--r-- lmao
drwxr-xr-x sub
-rw-r--r-- test.el
-rw-r--r-- test.sh
-rw-r--r-- toppings.txt
```

The special variable `$NF` is equivalent to the last field (NF stands for "number of fields") for each line:

```console
$ awk '{print $NF}' tmnt.txt
leader
hothead
party-animal
geek
```

Attempting to use a number < 0 results in an error. You are allowed to use a field number greater than what a line has, in which case, the empty string is returned for that line.


### Changing Field Delimiter


You can use the `-F` flag to change how awk delimits fields:

```shell
awk -F':' '{print $5}' /etc/passwd
```


## CHEATSHEET: Partial of `man wc`


```
NAME
       wc - print newline, word, and byte counts for each file

SYNOPSIS
       wc [OPTION]... [FILE]...
       wc [OPTION]... --files0-from=F

DESCRIPTION
       Print  newline,  word, and byte counts for each FILE, and a total line if
       more than one FILE is specified.  A word is a non-zero-length sequence of
       characters delimited by white space.

       With no FILE, or when FILE is -, read standard input.

       The  options below may be used to select which counts are printed, always
       in the following order: newline,  word,  character,  byte,  maximum  line
       length.

       -c, --bytes
              print the byte counts

       -m, --chars
              print the character counts

       -l, --lines
              print the newline counts

       -w, --words
              print the word counts
```


## CHEATSHEET: Partial of `man chmod`


```
NAME
       chmod - change file mode bits

SYNOPSIS
       chmod [OPTION]... MODE[,MODE]... FILE...
       chmod [OPTION]... OCTAL-MODE FILE...
```

Each comma-separated MODE is of the pattern `[augo][-+=][rwx]+`:

```shell
chmod a+x,u-r file1.txt file2.py
```

A single OCTAL-MODE string is 3 octal digits respectively representing the ugo permissions:

```shell
chmod 755 file3.sh file4.js
```

## Assorted Commands


### Some `ps` Flags


```shell
ps -efH
```

- `-e`: Select all processes.  Identical to -A.
- `-f`: Do full-format listing.
- `-H`: Show process hierarchy (forest).


### Creating File Links


```shell
# Hard link
ln TARGET LINKNAME
# Soft (symbolic) link
ln -s TARGET LINKNAME
```


### Other Commands


Refer to your Assignment 1 for commands like `sort`, `tr`, `comm`, `seq`.

Refer to your Assignment 2 for commands like `shuf`.

Refer to lecture notes for any other commands.
# Low-level Debugging


## Improving your Source Code


### Optimizing a Case Away


In GCC, there's a built-in function called `__builtin_unreachable()`. Your code should never call this function. The purpose of this is to mark a certain condition in your code as something that will never happen, so the compiler can optimize it away.

> **ASIDE:** Upcoming version **C23** has the function `unreachable()` which means the exact same, and this can be used in any compiler compliant with the spec.

```c
int sqrt(int n) {
  // Suppose you know by precondition n is non-negative
  if (n < 0)
    __builtin_unreachable();
  /* code */
}
```

This is a message from you, the developer, to the compiler telling it to never worry about the case of `n` being negative. This unlocks some optimization techniques, like something like `n >> 3` for division, which wouldn't work for negative numbers. Of course, if you don't ensure the precondition or abuse `__builtin_unreachable()`, you could cause undefined behavior. **The compiler trusts that you know what you're doing.**

Notice how there's no need for conditional branching that could slow down the code. You use an `if` statement, but the compiler optimizes it away because of what it means. No code gets generated for that block.

You could also transform this into a macro:

```c
#define assume(x) (x ? 0 : __builtin_unreachable())
```


### Maximizing Cache Efficiency


A technique for fitting variables into *cache lines* to maximize computations per cycle:

```c
// This means allocate x at a multiple of 16
// Arg to aligned() must be a power of 2
long x __attribute__((aligned(16)));
```

You can also mark functions based on how often they are likely to be called at runtime:

```c
// This function likely won't be called
int f(int) __attribute__((cold));
// The function likely WILL be called
int g(int) __attribute__((hot));
```

We want to divide code into two regions, "hot" and "cold". We want to make the hot region small, consisting of code we will execute a lot. Cold code can be as big as we want. The main point is that the code in the hot region will be *cached*. Cold code would sit in DRAM. This makes efficient use of our **instruction cache**.


## Improving your Binary with GCC Options


### Security Improvements


Attempt to protect against **stack overflow attacks**:

```shell
gcc -fstack-protector
```

This generates extra code in your function. It is as if you added something like:

```c
int f() {
  // added check
  int i = randomvalue; // some macro probably

  char buf[512];
  read(fd, buf, 512);
  x = buf[12];

  // added check
  if (i != randomvalue)
    abort(); // reliably crash, dump core

  return x;
}
```

This means that attackers would have to guess the random value, meaning they'd have to guess from the possible values `int` can take on, and even more if something like `long` is used.

This works because the attacker would have to overwrite the `randomvalue` to get to the bottom of the call frame to reach the return address, which is typically what they need to overwrite to continue their attack.

There is debate whether `-fstack-protector` is a default option. Some Linux distros like Debian and Ubuntu have it on by default. This is off by default on SEASnet. There's also the opposite option called `-fno-stack-protector` that turns the setting off.

> **ASIDE:** The **halting problem** states that there is no algorithm for looking at a program and figuring out if it would halt. There's no way to look at a program to figure out if it would stack overflow (Alan Turing).


### Performance Improvements


The `-O`, `-O2`, `-O3`, etc. flags to specify the level of *optimization **for CPU usage***.

```shell
gcc -O foo.c
```

This is off by default because it slows down the compiler. The compiler spends more time finding ways to optimize the machine code to make the *runtime performance* faster. It's a trade off between compile time and runtime performance.

Additionally, the higher the level of optimization, the harder it becomes to debug the machine code because the compiler may choose to take certain liberties for the sake of making the executable more performant, making the machine code less parallel with the original source code.

By default, it cares about runtime performance, not executable size. To *optimize for size*, you use the `-Os` flag.

There is another optimization option that is still under construction `-Og`. This means to *optimize for debugging* i.e. make the code as "debuggable" as possible without affecting performance.

The `-O0` flag specifies to *not optimize at all*.

---

**ASIDE:** Consider this case where turning optimization on breaks the code:

```c
double *p;
*--p = *p++ * *p++;
// "pop pop multiply push"
```

If you have competing side effects in the same statement, this is *undefined behavior*. The compiler is allowed to perform the actions in whatever order.

The compiler "activated" the bug by relying on undefined behavior in the name of optimization.

---


### Automating Hot/Cold with Profiling


1. You compile the program with a special flag that inserts extra code to count the number of times each instruction is executed. This understandably slows down execution.
2. You then run the program to gather the statistics.
3. Then, you recompile the program *with* the statistics. That way, the compiler knows which instructions are hot and cold and can decided how to mark them itself.

As long as your test runs are representative of how your program will actually be run, this is a great way of improving the performance of your application.

---

**Other GCC options**

```shell
gcc --coverage
gprof
```

---


### Inlining


```shell
gcc -flto
```

Normally, GCC optimizes one module at a time.

```c
// m.c
static int f(int x) {
  return -x;
}
```

Then suppose in the same module you call:

```c
// m.c
f(3);
```

The compiler can deduce at compile-time that `f(3)` will always resolve to `-3`, so it can just expand it to `-3`. This is called **inlining**.


### Link-Time Optimization


Then suppose you have definition and call in different modules:


```c
// m.c
int f(int x) {
  return -x;
}
```

```c
// n.c
extern int f(int x);
```

```c
// n.c
f(3);
```

By default, the compiler can no longer make this expansion. It instead has to go through the overhead of a normal function call: putting `$3` into a register, etc.

The point of `gcc -flto` is to instruct the compiler to perform the optimization *across module boundaries* - whole program optimization.

```shell
gcc -flto foo.c bar.c baz.c
```

This creates a `foo.o`, `bar.o`, etc. that contains machine code, but within the file is also a copy of the source code. Thus, by the time you link them together:

```shell
gcc -flto foo.o bar.o baz.o
```

GCC has a copy of the entire source code, so it can perform the inline optimization to generate better code. The *downside* is that this command takes a long time. The optimization algorithms that GCC uses are around $O(N^2)$. It could take literal days with a large program.


## Catching Bugs with Static Checking


Debugging your program *before* the program runs. "Static" here has nothing to do with static allocation/static lifetime; it just means when the program isn't running. This is the opposite of **dynamic checking**.

* ➕ No runtime overhead.
* ➕ Covered bugs are 100% prevented by the time the program runs.
* ➖ Some bugs can not be statically checked for 100% reliably.

Example of static checking:

```c
static_assert(0 <= n);
```

The implication however is that the expression e.g. `0 <= n` MUST be computable at compile-time. If it isn't, it's a compiler error.

**EXAMPLE:** Suppose you have a program that assumes a `long` is 64 bits:

```c
#include <limits.h>
static_assert(LONG_MAX >> 31 >> 31 == 1);
```

This is once again like a *message from you to the compiler*. The compiler then checks beforehand for this condition before attempting to compile the program.


### GCC Warning Flags


In general, the flags beginning with `W` mean "warning". `-Wall` means "turn on all warnings":

```shell
gcc -Wall
```

However, people found that this tends to output extraneous/unnecessary warnings. Nowadays, `-Wall` has come to mean "turn on all warnings that most people will find useful."

`-Wall` implies:

* `-Wcomment`: warn about valid but bad comments like `/* a /* bad comment */`

* `-Wparentheses`: warn about style considered to be bad because something probably forgot parentheses in situations where operator precedence may be less familiar, like in  `i << j + k` or `i < j || k < l && m < n`. The latter is common knowledge, but GCC disagrees, so this flag can be controversial.

* `-Waddress`: warn about making pointer comparisons when you probably didn't mean to, like `p = strchr("ab", "b"); if (p == "b") ...`

* `-Wstrict-aliasing`: warn about trying to "cheat" with pointers.

  ```c
  int i;
  long *p = (long *)&i;
  ```

  The C/C++ standard states that this results in undefined behavior, even if `int` and `long` happen to be the same size. This is controversial because a lot of low-level programs like the Linux kernel uses this all the time, *very carefully*. This is also why **casting** in general is risky because optimizing compilers may not do what you expect.

* `-Wmaybe-uninitialized`: warn about possible paths through a function where you use an uninitialized variable.

  ```c
  int f(int i)
  {
    int n;
    if (i < 0)
      n = -i;
    return n; // if i >= 0, uninitialized n is returned
  }
  ```

  If you do something that checks out with arithmetic reasoning, the compiler may or may not be find with it, depending on other flags you pass it.

  ```c
  int f(int i)
  {
    int n;
    if (i < 100)
      n = -1;
    if(i <= -1)
      return n;
  }
  ```
  Thus, there could be false positives if the compiler is not smart enough to deduce that the combination of things you've done always returns an initialized variable.


### GCC Extra Warning Flags


`-Wextra` is a collection of warnings like `-Wall` that are more controversial/less useful.

Some flags it includes are:

`-Wtype-limits`: warn about comparisons where the answer is obvious due to the type.

```c
unsigned u;
if (u < 0)
  /* This code would never run! */
```

This is not always trivial. For example, if you're trying to run portable code, some `typedef`s might be different:

```c
#include <time.h>
time_t t;  // could be signed or unsigned
if (t < 0)
  /* This code may or may not run! */
```

In recent versions of GCC:

```shell
gcc -fanalyzer
```

This turns on **interprocedural warnings**. This looks at all callers and callees of a function to come up with a better picture of, say, if a variable is uninitialized or not. In other words, it looks through all *all paths through all calls* of every function (in current .c file, as that is what is deducible at compile-time). This is disabled by default because it slows the compiler down.

Specifying both of these flags would theoretically be rewarding but very expensive:

```shell
gcc -fanalyzer -flto
```


## Helping the Compiler through Source Code


You can modify your source code in minor ways to help the compiler do better checking.


### _Noreturn


```c
// Prototype for some function that will never return
_Noreturn void fatal_error(char const *);

size_t size_sum(size_t a, size_t b)
{
  if (a <= SIZE_MAX - b)
    return a + b;
  fatal_error("size overflow");
}
```

Without the special `_Noreturn` signature, GCC will complain because it thinks size_sum is not always returning `size_t` even though it always does because `fatal_error` exits the program.

This will be standardized in C23 as `[[noreturn]]`.

`_Noreturn` also lets the compiler check whether a function declared to not return actually doesn't:

```c
void fatal_error(char const *x)
{
  puts(x);
  exit(1);
}
```

`exit` is a `<stdlib.h>` function declared as something like:

```c
_Noreturn void exit(int);
```

So `fatal_error` as it is defined above is fine. If we were to leave off `exit(1)`, then GCC would know that something is wrong.


### Pure Functions


With GCC, you can declare a [**pure function**](Elisp.md#aside-concept-of-pure-functions) with the signature, telling the compiler that this function should not modify the state of the machine:

```c
int hash(char *buf, size_t bufsize) __attribute__ ((pure));
```

This is stronger than merely declaring the parameters as `const`. A pointer to `const` just promises to not modify an object via its pointer, not that its a pointer to an actual constant.

This way, if you do something like:

```c
char buf[100];
char c = buf[0];
i = hash(buf, sizeof buf);
// c == buf[0] must be true
// This gives the compiler opportunities to cache in register, etc.
```

This will be standardized in C23 as `[[reproducible]]`;


### Const Functions


Not to be confused with the `const` qualifier on C++ methods.

```c
int square(int) __attribute__ ((const));
```

This is an even stronger flavor of pure functions. The return value can not even depend on the current state of the function, only its arguments.

A benefit is that if you call a `const` function multiple times with the same arguments, it could optimize knowing they would return the same value.

This will be standardized in C23 as `[[unsequenced]]`.


### Multiple Attributes


<!-- I'm gonna be honest, I don't really get this example at all. -->

Example:

```c
// ... is a syntax feature for variable number of arguments
int my_printf(char const *fmt, ...) __attribute__
  ((nonnull(1) | format(printf, 1, 2)));
  // nonnull(1) means the first argument cannot be NULL
  // The format attribute does some type checking inside fmt arg:
  // my_printf("a=%s", 1024); // not okay!
```

The names inside the `__attribute__` label, like `nonnull` and `format`, are not actual functions. They are *keywords* to the `__attribute__` syntax.


## Catching Bugs with Dynamic (Runtime) Checking


An example of dynamic checking, where you check at *runtime* and abort or similar if something fails:

```c
#include <assert.h>
int f() {
  /* code */
  assert(0 <= n);
}
```

Checks you put in yourself. This is very flexible because it's your own code.

```c
#include <stdckint.h> // C23 only
if (ckd_add(r, a, b))
  return EOVERFLOW;
// Not sure what this is but yeah
```


### GCC -fsanitize


Alternatively, you can let the compiler *insert* runtime checking.

```shell
gcc -fsanitize=undefined
```

Generate extra code to make the program reliably crash if it attempts undefined behavior (except for addresses). This causes the program to be slightly slower.

At the machine level, the adding overflow may be implemented like:

```assembly
addl %eax,%ebx
jo   ouch       ; jump if overflow

ouch:
    call abort
```

This is the same as `-fsanitize=undefined`, but for addressing errors, such as subscripting an array. Due to technical reasons, this actually only catches *most* addressing errors, not all.

```shell
gcc -fsanitize=addresss
```

This attempts to insert runtime checks for **memory leaks**. Memory leaks are technically not undefined behavior nor errors.

```shell
gcc -fsanitize=leak
```

This attempts to insert runtime checks for **race conditions**, where there may be multiple threads that attempt to access the same I/O resource.

```shell
gcc -fsanitize=thread
```

Most of the time, these flags are useful for development only. They could be left in for production if the application prioritizes safety over performance.


## Valgrind


You can run a program called Valgrind on any program, no special compilation flags:

```shell
valgrind emacs
```

This runs the program in a special environment and allows for more runtime checking.

The summary screen displays information about memory leaks.

* ➕ No special flags needed when compiling.
* ➖ Much slower. Checking is also less extensive because Valgrind only has access to the machine code and not the underlying source code.

**BIG POINT:** Runtime checking is dicey. Even with all these tools, bugs can still slip through.

> **ASIDE:** You can manually raise a compiler error with preprocessor directives in C:
>
>```c
>#if INT_SIZE == LONG_SIZE
>  #error "ouch"
>#endif
>```


## Portability Checking


Portability is a large part of software construction issues.


### Portability Concerns


* **Architecture**: 32 vs. 64-bit platforms?

```shell
gcc       # default is 64-bit
gcc -m32  # check for 32-bit
```

* **OS:** Linux vs. MacOS vs. Androids vs. ...?
* **Software:** Chromium vs. Frefox vs. ...? (JS)
* **Software version:** Chromium v102 vs. v107 vs ...?

You need a good strategy to tackle this. One strategy is to have a comprehensive testing system. But you also need to consider the source code. The way it's done is defining a **portability layer**, a level of abstraction:

```
main code
    |
(some API)
    |
portability module
    |
Chromium, etc.
```

> The compiler should be your servant, not your master. If you get warnings, look at what they're actually saying. If they're false positive, shut them off. **- Dr. Eggert, probably**


## Debugging Strategies


1. You're better off NOT debugging at all. Often times projects are stuck in **debugging hell**, spending more time debugging than actually developing. Debugging is an inefficient way of finding and fixing bugs. If you're spending a lot of time debugging, you should probably change your software construction approach such that you don't get so many bugs. *Be proactive* e.g. use static checking!

2. Write test cases. **Test-driven development (TDD)** is a theory of development that states that test cases are higher priority than code; one should write test cases first, the idea being you can use the test cases to debug the specifications of the code before actually writing the code.

3. Use a better platform.

4. **Defensive programming**. "When you're driving, assume that everyone is an idiot, drunk or both." Assume the other modules are broken.

   1. You can use traces and logs (`print()` statements!).

   2. Checkpoint/restart. Have one primitive in your program that saves the state of your entire application, probably to some file where it persists. Then have another primitive that restores the state:

      ```python
      save_state("foo")  # do this periodically
      restore_state("foo")
      ```

   3. Assertions: crash when something that should never happen occurs.

   4. Exception handling.

   5. **Barricades**.

      ```
      +----------------+----------------+
      |    possibly    |   clean data   |
      |    bad code    |    structs     |
      +----------------+----------------+
                     ^
         some barricade that processes
         code from the outide world, which
         you assume to be bad, before it
         makes it into your code
      ```

   6. **Interpreters** (such as Valgrind) that execute code in a special environment.

   7. **Virtual machines** that run a program in its own special sandbox, isolated from the outside system.


> **TDD ASIDE:** If you write some test cases, and your program passes all the test cases, then you screwed up because you haven't found the bug you wanted to find. When you're writing test cases, you're trying to be imperfect. You're trying to think "how do I make this program crash?" Often times, tests are written by another class of developers because the self-interest of coders causes them to write bad test cases. **- Dr. Eggert, probably**


## Debugging Tools


Some examples are **GDB**, the GNU debugger, and [**Valgrind**](#valgrind).

**strace** is a command that outputs the system calls a program uses, as if there were many `printf` logging calls in the source code.

```shell
#      vvvvvvvvvvvvvvvvvvv any shell command
strace cat /etc/os-release
```

Other commands we've seen before like `ps` and `top` are also [*DevOps tools*](Cloud%20Computing.md). System administrators use this to monitor server activity, but they can also be useful for debugging.


## How Debuggers Work


GDB is actually a separate process from the process being debugged. It uses special system calls to exert some control over the process being debugged. Model:

```
(gdb process)--+
               | special system calls
               v
        (your process)
```

These special system calls include, at any point in execution:

* Starting/stopping/continuing
* Accessing memory
* Accessing registers

These do have security restrictions and cannot be used arbitrarily. These restrictions depend on the OS, but typically the rule is that the debugger must control a process with *same user ID*. Other OS may have a more restrictive rule, stating it can only debug a *child process*.


## Getting Started with Debugging: GCC


You can use a `-g` flag to specify debug info level for a C program:

```shell
gcc -g3 program.c
```

This conflicts with optimization because code that is optimized by the compiler tends to become harder to understand.

```shell
gcc -g3 -O2 program.c
```

This typically results in [**inlining**](#inlining), where calls to functions can be optimized away by substituting their body into places where it was called. The produced machine code would then be functionally equivalent but have lost the information that a function was even called.

What the `-g` flag does is bloat the resulting object and executable files with debugging tables. This data isn't visible when the program runs normally, but debuggers will be able to access them.

> **ASIDE:** `_FORTIFY_SOURCE` is a standard technique used by GCC to make stack overflows less likely to succeed. For technical reasons, this is incompatible with no optimization i.e. attempting `gcc -O0` will cause a compiler error.


## Starting GDB


Dropping a program into GDB:

```shell
#   vvvv program to debug
gdb diff
```

Setting the working directory of the program when it starts up:

```console
(gdb) set cwd /etc
```

Setting environment variables for the debugging session:

```console
(gdb) set env TZ America/New_York
```

A defense technique against buffer overflow attacks is to have the program run at randomized locations in memory (CS 33). By default, Linux executes programs in an environment with randomized addresses for the stack, heap, C library, etc. and many even the `main()` function.

The downside of this program is that it will run differently every time. This means that if there's a bug that depends on stack addresses for example, then it may appear sometimes and not for others. This makes debugging harder, so by default, this option is already on:

```console
(gdb) set disable_randomization on
```

Actually running the program. The arguments you supply after `run` are in shell syntax and forwarded to the executable being debugged:

```console
(gdb) run -u /etc/os-release - < /dev/null
```

*Alternatively*, you can make GDB be in charge of another program using the PID of running process, effectively suspending it.

```console
(gdb) attach 986317
```

Releasing the program:

```console
(gdb) detach
```


### Controlling Your Program


`^C` stops the program. GDB takes control.

`*(int *)0=27` crashes the program and falls under GDB's control.

Continue running the code:

```console
(gdb) # c
(gdb) continue
```

Single step through the source code. Similarly, single step through the machine code.

```
(gdb) # s
(gdb) step
(gdb) # si
(gdb) stepi
```

Stepping can be tricky because there isn't always a sequential mapping of source code lines to machine code lines. Stepping through some machine code lines may make it look like the program is jumping back and forth between source code lines instead of running one-by-one in order.

A courser-grained variant of the `step` command. Advancing to the next line of source code at the current function call level i.e. a single step but without worrying about function calls, stepping "over" them. Similarly, it has a machine code version.

All these commands control the **instruction pointer**, e.g. `%rip` on x86-64.

```console
(gdb) # n
(gdb) next
(gdb) # ni
(gdb) nexti
```

Finish the current function and then stop:

```console
(gdb) fin
```

You can use **breakpoints** to stop the program at a certain instruction, typically a function name. Creating a breakpoint:

```console
(gdb) # b
(gdb) break analyze
(gdb) break diff.c:19
```

Listing your current breakpoints and their numbers:

```console
(gdb) info break
```

Deleting a breakpoint by number:

```console
(gdb) del 19
```

You can use **watchpoints** to tell the program to run until the specified variable `p` changes value:

```console
(gdb) watch p
```

**How does GDB implement breakpoints?**

GDB takes the process being debugged and modifies its machine code. It stomps on the machine code of the specified function/line/instruction by zapping the first byte with a special instruction that is guaranteed to cause the program to trap, allowing GDB to take control.

**How does GDB implement watchpoints?**

Single step through the code, and after each instruction, see if `p` has changed. This can be really slow unless youh have special hardware support for watchpoints. Many CPUs, including x86-64, have this support.


### Other GDB Commands


Printing a C expression (or register values):

```console
(gdb) # p
(gdb) print expr
(gdb) print $rax
(gdb) print a[5]
(gdb) print cos(3.0)
```

It does more than just allow you to look at data. It lets you run a subroutine like `cos` in the program, which can modify the data and/or call arbitrary code from other parts of the program.

Disassembling a function to get the assembly code:

```console
(gdb) disas cos
```

You can set a **checkpoint** and then run the code from the checkpoint by its number:

```console
(gdb) checkpoint
(gdb) restart 42
```

The inverse of `continue` is `reverse-continue` (`rc`). This means to start running the program backwards until it hits the most recent breakpoint that it passed. This tends to be *very expensive* because GDB has to set a bunch of checkpoints under the hood.

```console
(gdb) # rc
(gdb) reverse-continue
```

For **cross-debugging**, you can specify what target you want to run in

```console
(gdb) target
```

<!-- IDK lol. -->

This makes GDB run on some virtual machine or something?


<script type="text/javascript" src="http://cdn.mathjax.org/mathjax/latest/MathJax.js?config=TeX-AMS-MML_HTMLorMML"></script>
<script type="text/x-mathjax-config">
  MathJax.Hub.Config({ tex2jax: {inlineMath: [['$', '$']]}, messageStyle: "none" });
</script>
# The Python Programming Language


## History of Python


From `grep` to `sed` to `awk`, such commands attempt to generalize and expand what the previous does, rising in levels of complexity.

The **Perl** programming language was designed to be able do everything awk, etc. can do.

Then **Python** was designed to do everything Perl, etc. can do. The rise of Python can be attributed to two parts:


### Part 1: BASIC and ABC


**BASIC**: an instructional language. Instructors noticed that students showed up knowing BASIC.

Instead of writing very low-level code, go up one level of abstraction. Give people a language where:

* Hash tables are implemented into the language, like `set` and `dict`
* All the basic algorithms already built-in. Just `sort` lol.
* Enforce indentation.
* An IDE to run the programs.

High school students will then be programming in this new language called **ABC**. It didn't work because employers still wanted BASIC.


### Part 2: The Perl Programming Language


> Perl  = sh + awk + sed + ...

Perl was designed as an antidote to the "Little Languages" philosophy, so it combined all the little languages into one scripting language. Perl became the scripting language of choice for about 10 years. It was designed to be like a real spoken language - there was always more than one way to do something.


### A Combination


However, ABC had the philosophy that there is one correct way to do anything.

Python emerged as a combination of:

* The philosophy of ABC. From [The Zen of Python](https://peps.python.org/pep-0020/#the-zen-of-python):

  > There should be one-- and preferably only one --obvious way to do it.

* The capabilities of Perl. Python is a scripting language that tries to do everything. Theoretically, if you know the language very well, you do not have to touch the little languages of the shell. Python crutches FTW :D (very not biased)

> **ASIDE:** All 3 languages, BASIC, Perl, and Python can be either compiled or interpreted.


## Why Python?


* **CONS:** Slow, memory hog.
* **PROS:** Easier to write (development cost vs. runtime cost). A lot of libraries are also written in C/C++ code for a performance bonus, made possible by **native method interfaces**.

Scripting languages like Python demonstrate an alternate balance between **development cost** and **runtime cost**. Often times, human time is much more valuable than computer time.

Prevalent in machine learning, although this feat is probably due to being at the right place at the right time. Python happened to be a reasonable scripting language for the job when the field was emerging in popularity.

Oh yeah and here's a quote I found pretty *profound*:

> There's always going to be a time where you're the blind person next to the elephant. The goal of software construction is to make you a better blind person. **- Dr. Eggert**


## Python Internals


Python is **object-oriented**. Historically, it didn't actually start out that way. It started with functions but no classes. When classes were introduced, they implemented *methods* as functions that explicitly take the `self` first argument, which is actually how OOP is implemented behind the hood in languages like C++.

Anyway, every value is an object. Every object has:

* Identity - *cannot be changed*
* Type - *cannot be changed*
* Value - *can* be changed, but only if the object is **mutable**


### Typing


In old Python, `int` used to have fixed size, so there was the distinction between integers and longs.

The main *categories* of types in modern Python:

<!-- Mixing in a little extra here from my own experience and how stuff was presenting in PIC  -->

**Singletons**: Types with only one instance throughout runtime

* `None`: Python's version of a `null` value
* `True` and `False`: Python's booleans, which (fun fact) actually subclass the numeric type `int`.

**Numbers**

* `int`: A number without a fractional part. In modern Python, these can be any size, so you don't need to worry about bounds/overflow like in most languages.
* `float`: A number with a fractional part. There's also the special `float("inf")` constant that represents infinity (`1/0` in C, `Infinity` in JavaScript, etc.).
* `complex`: A number with a real and imaginary part. You can initialize a literal with the `a+bj` syntax e.g. `5+4j`.

**Collections**

* **Sequential Containers**: Collections where order matters, with elements being accessed by **index**

  * `list`: Python's builtin vector type. Heterogenous, arbitrary-sized, ordered collections.
  * `tuple`: Same as `list`, but immutable, so it's fixed length. These are nice because they are more efficient and are in a sense "safer" to use.

* **Associative Containers**: Collections where elements are accessed by **key**

  * `set`: Python's builtin hash table. Unordered collection of unique, **hashable** values.
  * `dict`: Python's builtin hash map. Unordered collections of key-value pairs. Keys must be unique and **hashable**.

**Callables**: Objects that support being called with optional arguments

  * **Functions**: objects you define with the `def` keyword or anonymous ones with the `lambda` keyword.
  * **Methods**: function objects that have been bound to an instance of a class. They take a mandatory `self` positional argument, the class instance the method acts on behalf of.

There's also the **buffer** type. I assume this is only available in Python 2. Buffers are like multiple strings. When you're done working with it, you can convert it to a string with `str(x)`.


## Lists


<!-- I originally didn't have this section because I now know it by heart, but for completeness sake: -->

Common `list` methods:

* `my_list.append(value)`: add any value as an element to the end of the list.
* `my_list.extend(iterable)`: lay all the values out from the iterable as elements at the end of the list.
* `my_list.insert(index, value)`: insert an element at a position in the list, pushing everything after it backwards.
* `my_list.count(value)`: return the number of occurrences of `value` (using `==` checking).
* `my_list.remove(value)`: remove the first occurrence of `value` (using `==` checking), raising `ValueError` if not found.
* `my_list.pop([index])`: defaults to last item, which you can use along with `.append` to emulate a stack data structure. Raises `IndexError` if empty.
* `my_list.clear():` remove all elements.

Common operations universal to sequential containers:

* `len(my_list)`: return the number of elements.
* `my_list[i:j]`: return a `slice` of the container, starting from index `i` and up until but excluding `j`.
  * Mutable ones also support this syntax on the LHS, where it means **reassigning** a segment of the container, as well as `del my_list[i:j]`, which deletes that segment of the container.


### ASIDE: Underying `list` Allocation


* Probably uses cache size to determine starting size.
* After that, reallocation uses geometric resizing (approximately nine-eights according to [mCoding](https://www.youtube.com/watch?v=rdlQzhP71pQ)).
* The total cost of calling `list.append` N times is $O(N)$. Because the **amortized cost** of this operation is $O(1)$.

**Visualization:** imagine that the list length is doubled for every allocation, which isn't true, but this doesn't change the asymptotic time, so it simplifies the derivation:

```
[e e e e e e e e e e e e e e e e e e e e e e]
  ... |<3>|<--2 ops-->|<-------1 op-------->|
```

The total cost is $\frac{1}{2}*2 + \frac{1}{4}*2 + \frac{1}{8}*3 + ...$, which converges to 2, which is $O(1)$.


## Python vs. Shell vs. Emacs Scripting


Lisp is an ASL for Emacs, like an extension language. It uses existing code, *Emacs primitives*.

Shell uses existing programs.

Python was designed to be a *general-purpose programming language*, so there are no "primitives" you bring together - you just write in the language altogether to program from scratch. However, it also converges to the same phenomenon where programmers glue together existing modules like PyTorch and SciPy.

What makes a language a scripting language is one that supports this pattern of software construction of building applications from existing code.

Another quote I found quite *profound*:

> The goal of a **scripting language** is you don't code from scratch. You glue together other people's code. You provide the cement, and the other people provide the bricks. **- Dr. Eggert**


## Classes and OOP


Class hierarchies are **directed acyclic graphs (DAG)**. This is especially apparent because unlike languages like Java, Python supports **multiple inheritance**.

Python's **method resolution order (MRO)** is depth-first, left-to-right. So for example, if you define a class that inherits like so:

```python
class C(A, B):
    def some_method(self, arg):
        pass
```

With the DAG model, this design makes it so that if `A` and `B` disagree, `A` will always take priority.

**Historical ASIDE:** The decision to explicitly include `self` in all method definitions was to not abstract a fundamental mechanism of OOP: every method is *bound* to the class and *acts on* the instance. If you examine the machine code of similar OOP languages like C++, you'll see that there's a hidden first argument to every method, that is the pointer to the object that the method is acting *on behalf of*.

<!-- From one of my Piazza answers -->

**Introspection ASIDE:** You can use the built-in `__mro__` attribute of class objects to programmatically access a class' method resolution order. For example:

```python
# Print the names of the classes in class O's MRO
print([cls.__name__ for cls in O.__mro__])
```


### Dunders and "Operator Overloading"


Besides the ones you already know...

The old way to redefine the comparison operators:

```python
def __cmp__(self, other):
    # negative for <, 0 for equal, positive for >
    return num
```

This is still supported but it is now anachronistic approach because you can run into hardware problems. A notable example is the case of *floating point numbers*, which have an additional state, **NaN**, beyond negative, zero, and positive. Thus, we have the familiar `__lt__`, `__gt__`, etc.

This is the Python 2 predecessor to the familiar `__bool__` method:

```python
def __nonzero__(self):
    # Return whether the object is considered to be "not zero"
    return b
```


### Namespaces in Classes


**Namespaces** are just dictionaries. Classes have a special attribute `__dict__`, a `dict` that maps names to values. This gives rise to opportunities to write "clever" Python code, where you can programmatically alter the definition of an existing class:

```python
c = C()
c.__dict__["m"] is c.m
```

This is (probably) how **metaclasses** are implemented, which was not mentioned in lecture but is cool to know. They're basically classes that determine how other classes should be implemented.


## Modules


### The `import` Statement


1. Creates a namespace for the module.
2. Executes the contents of the module *in the context of that namespace*. Eggert didn't mention this, but this step is actually only performed if the module *hasn't already been imported*. Modules are only run onces.
3. Adds a name, the module name, to the current namespace.

Proof for #2:

```python
# module.py
print("Hello world")
```

```python
# runner.py
import module
import module
```

```console
$ python3 runner.py
Hello world
```


## Packages


Packages organize source code into a familiar tree structure. This allows importing to be parallel the file system.

The special `__init__.py` scripts turns a directory into a proper package, and it is automatically run upon import.


### The `PYTHONPATH` Environment Variable

Just as how [`PATH` instructs the shell program where to search for commands](Shell.md#path-and-resolving-command-names), `PYTHONPATH` instructs the Python interpreter on where to search for code.

Determines the behavior of the `import` statement. Python will search through the sequence of paths, delimited by colons (Unix) or semicolons (Windows), to search for names of packages or modules to import. The path to the standard library is included in `PYTHONPATH` by default.

Official documentation: https://docs.python.org/3/using/cmdline.html#envvar-PYTHONPATH.

This variable is stored in and can be modified programmatically with `sys.path`, which is a `list[str]` containing the individual string paths.

**Why all this complexity with packages vs classes?**

Packages are oriented towards developers (like a *compile-time notion*). The tree is structured so that different developers can work on different parts of the code.

Classes are about runtime behavior (a *runtime notion*). You want inheritance to be independent of package hierarchy. Classes are only concerned with their own behavior, "what to do next", so it should be able to pull code from anywhere in the codebase. How developers *organize* that codebase is made possible with packages.


<script type="text/javascript" src="http://cdn.mathjax.org/mathjax/latest/MathJax.js?config=TeX-AMS-MML_HTMLorMML"></script>
<script type="text/x-mathjax-config">
  MathJax.Hub.Config({ tex2jax: {inlineMath: [['$', '$']]}, messageStyle: "none" });
</script>
# The Shell


## Operating Systems


* Ubuntu is an example of a Linux distro (distribution).
* Debian is the upstream distro for Ubuntu from which it inherits thousands of *packages*. You can can check the packages with `dpkg -l`.
* Linux is an OS **kernel**.

**Software levels of abstraction:**

```
+------------------------+
|          apps          |
+------------+-----------+
|            |   libs    |
|   kernel   +-----------+
|            | C stdlib  |
+------------+-----------+
|        hardware        |
+------------------------+
```

sh and Emacs and any of their independent instances are themselves **applications**, one of many that sit atop the operating system.

**Introspection**: When a program looks at itself ("when we use tools to find out more about our tools"). Knowing how to perform introspection is a portable, universal skill that lets you explore or relearn something about an unfamiliar program.


### Superusers and the Concept of Privilege


* **Superusers** ("root") are the only ones with permission to `kill` PID 1, `system`.
* The **sudo** command lets you run a command AS "root":

```shell
sudo sh
```

**Why have multiple users instead of just root?**

The concept of **minimization of privileges** aka "principle of least privilege". If a program breaks or a user makes a mistake, the damage is limited.

By the way, you can kill a program by PID with:

```shell
kill PID1 PID2 ...
```


## The `(ba)sh` Program


A shell is itself a program, and programs themselves are just files that can be executed by the operating system.

**sh** is the predecessor to **bash** (Bourne Again SH). sh was designed to work on 16-bit machines so it's a very little language. Bash adds some features in addition ot the original sh.

There is also a lot of other shell languages ending in sh. Having so many distinct shell languages becomes a problem, so the **POSIX standard** was created as a spec for shells.

Creating another instance of the shell from within the shell itself to execute a one-off command:

```shell
sh -c <command>
```

This is what you call a **subprocess**, or a **child process**. Within your running instance of `sh` (the CLI you're typing into), another *instance* of `sh` is spawned as a child of that `sh`. In fact, every time you run a command, an instance of their little program is attached to your shell as a child process. You can see this for yourself when using commands like [`ps`](#the-ps-program) to show processes and their parentage.


### ASIDE: Some Mischievous Things


**Things you can do to annoy your system administrator*:*

Telling the shell to go into an infinite loop:

```shell
sh -c 'while true; do true; done'
```

A no-op for a number of seconds, doesn't use CPU

```shell
sleep 10
```

The `truncate` command sets a certain file to a certain size, ending at the size you specify (filling with null data if the size expands I presume):

```shell
truncate --size=10TB bigfile
```

If you inspect the filesystem, you'll find that you didn't actually use up that much space, just convince the directory listings that that much space had been allocated for *something*. This is still annoying though because it will trip up the sysadmins when they perform **backups**.


## Shell Commands


### Basic Output Manipulation


Some basic commands and their most common arguments you should integrate into your workflow (and ones that will be used without warning from here on out). These are typically used as something to pipe output *into* to improve your experience:

* `wc [-cmlw]`: Output statistics about the number of bytes (`-c`), characters (`-m`), lines (`-l`), and/or words (`-w`) of the input stream or file(s).
* `head -n N`: Output only the first `N` lines of the input text.
* `tail -n N`: Output only the last `N` lines of the input text.
* `more`: Paging utility that lets you scroll through text a screenful at a time instead of having it be outputted all at once to the console. Like a read-only editor, it also supports interactive commands reminiscent of Vi keybindings and features like regex search.
* `less`: The direct upgrade to `more`, it also supports backwards navigation and more inclusive support for keybindings.


### Command Arguments


Within shell languages, the simplest commands look like:

```
word0, word1, ..., wordn
```

`word0` is the name of the program and `word1, ..., wordn` are the arguments to that program. Arguments typically follow conventions:

* Normal (**positional**) arguments, like `a`.
* **Options** (aka **flags**) `-ejh` which is also equivalent to `-e -j -h`. Jamming them together is a thing you can do on Unix, but as we know from Assignment 2, this behavior becomes tricky when there are options that take arguments.
* Options that *don't* take any arguments are sometimes referred to as **switches**, and they set some kind of configuration simply by being present or absent (think of it like a boolean option, where presence means true and absence means false).


### Command Substitution


<!-- NOTE: I previously referred to this as "sub-expression", which is easily understood in general speak, but that's not actually the formal terminology. -->

The **command substitution** `$()` syntax allows you to write a shell command *as if you wrote the output* of the sub-expression word for word out as the arguments to the outer command.

Note that this can have significant edge cases like in this example from lecture:

```shell
mkdir empty
cd empty
wc $(find . -type f)
```

The program then looks like it's jammed because if the sub-expression `$(find . -type f)` returns nothing, it's *as if* you just typed the command:

```shell
wc
```

`wc` has defined behavior where if you did not provide any command line arguments, then it reads from stdin. Thus, the program is actually prompting the user to input something to the `wc` program.

Only caring about that last summary line, you can use the `tail` command:

```shell
wc $(find . -type f) | tail -n 1
```


### Running Multiple Commands


**Commands in Sequence**

`;` is the **sequencing operator**, equivalent to a newline but allows you to automatically queue subsequent commands in one line if you're at the command line:

```shell
ls -d .; echo a
```

This forms the basis of shell **scripting** - a sequence of commands to automate certain tasks.

> A big part of scripting is knowing when you don't have to care about efficiency. "Your time is more important than the computer's time." This is a different paradigm then coding with algorithms and asymptotic time, etc. in mind.

**Commands in Parallel**

```shell
ls -d . & echo a
```

**Command Pipeline**

**Piping** output of one command as the input to another. The **pipe** is itself a buffer. Commands that write can write to it, commands that read can read from it, so you can set up a sequence of writing and reading commands to **pass content between them**:

```
ls -d . | less
```

Instead of `ls -d` running until completion and *then* running `less`, *every time* `ls -d` outputs something to the buffer, `less` can read from it and execute its program.

This is **massively useful** because it allows you to set up a sequence of little languages that each process or transform a single stream of text output as it makes its way into some final form.

Example from discussion:

1. Listing all files ended with `.html` under ~/...
2. ...sorting it...
3. ...and then outputting it into a file named `list_html.txt`:

```shell
ls ~/*html | sort > list_html.txt
```


## The `ps` Program

* Like the task manager: lists processes and their information
* For a live view, use the `top` program instead
* You can also use `less /proc/cpuinfo` to view information about the currently running processor
* By default lists the processes on the local machine, but you can use some useful flags like:

```shell
ps -ejH | less
```

You can pipe the output into the [grep command](#the-grep-command-and-pattern-matching) to filter it by a keyword, as if searching for a process by name:

```shell
ps -ef | grep emacs
```


## PATH and Resolving Command Names


If the name of a program does not contain any slashes, then the shell automatically looks through directories listed in the `PATH` **environment variable** to find the program name. This is why when you want to run an executable in the current directory, you have to use the `./executable` syntax:

```console
$ a.out
bash: a.out: command not found
```

```console
$ ./a.out
Hello world
```

Alternatively you can *append* the current directory to the PATH:

```shell
PATH=$PATH:.
```

Now it would work because `a.out` is now treated like a command that's on your `PATH`:

```console
$ a.out
Hello world
```

Had to say it one last time for y'all:

> For the last time, make sure `/usr/local/cs/bin` is prepended to your `PATH`!

Y'all better understand why that's so important now. Cheers.

<!-- I kind of forgot what this aside was in reference to: -->

> You can type the *null byte* with `^@`! Alternatively, you can use `printf '\0'` to use familiar C code and pipe that into whatever you want.


## Quoting


The shell itself has special characters, so you must use **quoting** to input what you intend to. A common use case is quoting a file name that has a space in it so the shell does not treat the space as a delimiter for arguments.

Special characters include *whitespace* as well as:

```
` ~ # $ & * ( ) = [ ] \ | ; ' " < > ?
```

There are two forms of quoting: single (`'`) and double (`"`).


### Single Quoting


Using single quotes makes the shell treat everything inside the quotes as a *single word*. More importantly, *every* character inside is preserved *literally*. This means that you cannot include the single quote `'` itself because escaping is not possible.

```console
$ echo 'hello\there\n"general kenobi"'
hello\there\n"general kenobi"
```


### Double Quoting


Alternatively you can use *double quotes* to enclose "most any characters". `$` is still special, used to **interpolate** other variables or sub-expressions. Special characters are also possible with **escape sequences**, like `\"` to represent a double quote mark itself. Interestingly, `\n` does not work the way you would expect.

```console
$ echo "hey there's\n\"general kenobi\""
hey there's\n"general kenobi"
```

Quoting can even include newlines itself (RET at the command line), but that's so controversial that it's probably going to be removed in an upcoming POSIX standard release.


## Globbing (Wildcard) Pattern Matchinig


<!-- This section used to be part of the Midterm discussion review in week4.md. -->

Used in file management: `man 7 glob`

The shell will **expand** strings containing these special characters to every string that matches the pattern, separated by whitespaces. For example, `*` in a directory containing files named `file1`, `file2`, and `file3` would expand to:

```console
$ echo *
file1 file2 file3
```

This is done *by the shell* and NOT the programs typically associated with them like `ls`. When you run something like `ls hello/*`, bash first expands the string `hello/*` to `hello/some_file hello/another_file ...` and *then* passes it to `ls`.

- `?` - match one, any character
- `*` - match any number of characters, including the empty string
- `{pdf,jpeg}` - match multiple literals
- `[]` - character set that supports *ranges*, similar to regex
- `[!]` - complement of a character set if `!` is the first character
- `\` - escape character


## Shell Scripting Constructs


### Looping


```sh
for i in {1..100}
do
    echo $i
done
```


### Conditionals

```sh
if [ condition ]
then
    # body
fi
```

You can use the `expr` command to evaluate an expression. You can also use **compound expansion** with `((expression))` syntax.
