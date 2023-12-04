# Nsumo
This repository tracks the code of Nsumo (500g 10x10cm Sumobot) as I re-write it
from scratch and show the complete process in a [YouTube series](https://www.youtube.com/watch?v=g9KbXJydf8I&t=1s).
You find the hardware design [here](https://github.com/artfulbytes/nsumo_hardware.git).
The project is meant to serve as an educational example and case study of a
microcontroller-based embedded project.

<img src="/docs/nsumo.jpg">

## Directory structure
The directory structure is based on the
[pitchfork layout](https://github.com/vector-of-bool/pitchfork), and I describe it
more in [this video](https://www.youtube.com/watch?v=6oJ2LLxfP3s).

| Directory    | Description                                                  |
|--------------|--------------------------------------------------------------|
| build/       | Build output (object files + executable)                     |
| docs/        | Documentation (e.g., coding guidelines, images)              |
| src/         | Source files (.c/.h)                                         |
| src/app/     | Source files for the application layer (see SW architecture) |
| src/common/  | Source files for code used across the project                |
| src/drivers/ | Source files for the driver layer (see SW architecture)      |
| src/test/    | Source files related to test code                            |
| external/    | External dependencies (as git submodules if possible)        |
| tools/       | Scripts, configs, binaries                                   |
| .github/     | Configuration file for GitHub actions                        |

## Build
The two most common ways to build code for a microcontroller are from
an IDE (often supplied by the vendor) or from the command-line through a build
system (for example, make). There are pros and cons to both ways, and I describe
them in [this video](https://www.youtube.com/watch?v=H1HToCzku9Y) and
[this video](https://www.youtube.com/watch?v=HCfq44NNBaU).

In short, using an IDE is the most straightforward approach, but
at the same time less flexible because it forces you into a particular environment.
A build system is more complicated to set up, but it can be combined with your editor
of choice, gives more control over the build process, is great for automation (see CI),
and allows for a command-line based environment, which transfers better across projects.
This project can be built both ways, however, _make_ is the preferred way, but the IDE
is helpful when step-debugging the code.

## make (Makefile)
The code targets the MSP430G2553 and must be built with a cross-toolchain. While
there are several toolchains for this microcontroller, msp430-gcc is the one used in this project,
which is available on [TI's website](https://www.ti.com/tool/MSP430-GCC-OPENSOURCE).
Watch [this video](https://www.youtube.com/watch?v=HCfq44NNBaU) and
[this video](https://www.youtube.com/watch?v=dh1NFAIoZCI) for more details.

There is a _Makefile_ to build the code with _make_ from the command-line:
``` Bash
TOOLS_PATH=<INSERT TOOLS PATH> make HW=<INSERT TARGET>
```

The path to the toolchain must be specified with the environment variable _TOOLS_PATH_.

The argument _HW_ specifies which hardware to target, either _NSUMO_
(Robot with 28-pin msp430g2553) or _LAUNCHPAD_ (evaluation board with 20-pin
msp430g2553).

For example, if the toolchain is available at _/home/artfulbytes/dev/tools/msp430-gcc_,
and targeting the evaluation board:

```
TOOLS_PATH=$HOME/dev/tools make HW=LAUNCHPAD
```

## IDE
The IDE provided by the vendor (TI) for the MSP430 family of microcontrollers is
called Code Composer Studio (CCSSTUDIO). It's an eclipse-based IDE, and is available
at [TI's website](https://www.ti.com/tool/CCSTUDIO). The below steps have
been verified to work with version CCSTUDIO 11.1 and MSP430-GCC 9.3.1.11.

1. Install CCSTUDIO and MSP430-GCC, and make sure the GCC compiler can be selected
   from inside CCSTUDIO.
2. Create a new CCS project
    - Target: MSP430G2553
    - Compiler version: GNU v9.3.1.11
    - Empty project (no main file)
3. Drag the src/ folder into the project explorer and link to files and
   folders
4. Under properties
    - Add src/ directory to include paths
    - Add define symbols (see Makefile), e.g. LAUNCHPAD

## Tests
There are no unit tests, but there are test functions inside src/test/test.c, which
are written to test isolated parts of the code. They run on target but are not built
as part of the normal build. Only one test function can be built at a time, and it's
built as follows (test_assert as an example):

``` C
make TARGET=LAUNCHPAD TEST=test_assert
```

## Pushing a new change
These are the typical steps taken for each change.

1. Create a local branch
2. Make the code changes
3. Build the code
4. Flash and test the code on the target
5. Static analyse the code
6. Format the code
7. Commit the code
8. Push the branch to GitHub
9. Open a pull-request
10. Merge the pull request (must pass CI first)

This workflow is described further in [this video](https://www.youtube.com/watch?v=dh1NFAIoZCI).

## Commit message
Commit messages should follow the specification laid out by
[Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/). See
[this video](https://www.youtube.com/watch?v=TFhbv6gw2Wo) for more details.

## Continuous Integration (CI)
There is simple continuous integration (CI)
system in place to have some form of protection against breaking code changes. This system is realized
through GitHub actions and is configured in **ci.yml** located under
**.github/workflows**. The action builds and analyses (cppcheck) each commit pushed to GitHub and blocks
them from being merged until they pass. The action runs inside a docker container, which has the
required cross-toolchain installed. The dockerfile is located under **tools/**, and the docker image is
available in a [repository at Docker Hub](https://hub.docker.com/r/artfulbytes/msp430-gcc-9.3.1.11).
The CI system is described in detail in [this video](https://www.youtube.com/watch?v=dh1NFAIoZCI).

Note, the CI system provides weak protection against breaking code changes since it only builds and
static analyses the code. It's mainly there to demonstrate the principle. A more rigorous CI system
(professional environment) would unit-test the code and test it on the real target.

## Code formatter
The codebase follows certain formatting rules, which are enforced by the code formatter
**clang-format**. These rules are specified in the **.clang-format** located in the root
directory. There is a rule in the **Makefile** to format all source files with one command
(requires clang-format to be installed).

``` Bash
make format
```

Sometimes it's desirable to ignore these formatting rules, and this can be achieved with special comments.

``` C
// clang-format off
typedef enum {
    IO_10, IO_11, IO_12, IO_13, IO_14, IO_15, IO_16, IO_17,
    IO_20, IO_21, IO_22, IO_23, IO_24, IO_25, IO_26, IO_27,
    IO_30, IO_31, IO_32, IO_33, IO_34, IO_35, IO_36, IO_37,
} io_generic_e;
// clang-format on
```

## Coding guidelines
Apart from the basic formatting rules, the codebase also follows certain coding guidelines.
These are described in **docs/coding_guidelines.md**.

## Static analysis
To catch coding mistakes early on (in addition to the ones the compiler catches), I use a static
analyzer, **cppcheck**. There is a rule in the **Makefile** to analyse all files with **cppcheck**.

``` Bash
make cppcheck
```

I explain it more in [this video](https://www.artfulbytes.com/analysis-with-cppcheck).

## Memory footprint analysis
The memory footprint on a microcontroller is very limited. The MSP430G2553 only has
16 KB (16 000 bytes) of read-only memory (ROM) or flash memory. This means one has to
be conscious of how much space each code snippet takes, which is one of the challenges
with microcontroller programming. For this reason, it's useful to have tools in place
to analyse the space occupied. There are two such programs available in the toolchain
(_readelf_ and _objdump_), and two corresponding rules in the _Makefile_.

One rule to see how much total space is occupied,
```
make size
```
and another rule to see how much space is occupied by individual symbols (functions + variables):
```
make symbols
```
which is useful to track down the worst offenders.

## Assert
Several things happen when an assert occurs to make it easy to detect and localize.
First it triggers a breakpoint (if a debugger is attached), then it traces the address
of the assert, and finally it endlessly blinks an LED. The address printed, is the
program counter, and _addr2line can be used to retrieve the file and line number,
and there is a makefile rule for it. For example, if an assert triggered at address
0x1234, you can run

```
make HW=LAUNCHPAD addr2line ADDR=0x1234
```

## Diagrams
There are some PlantUML diagrams under _docs/_. The plaintext can be converted into
a viewable image with

```
java -jar plantuml.jar diagram.uml
```

Download plantuml.jar from [PlantUML website](https://plantuml.com/), and install
dependencies (on Ubuntu) with

```
sudo apt install default-jre
sudo apt install graphviz
```

## Schematic
<img src="/docs/schematic.png">

## Software architecture
<img src="/docs/sw_arch.png">

## Block diagram
<img src="/docs/sysdiag.jpg">

## State machine
<img src="/docs/state_machine.png">
<img src="/docs/retreat_state.png">
