This repository contains tools for generating and working with Reference Structures.

#Installation

##With Docker

The correct way to install and run the sequence graph tools is through Docker. [This repository](https://github.com/adamnovak/sequence-graphs) is automatically built into a [Docker container on the Docker Hub](https://registry.hub.docker.com/u/adamnovak/sequence-graphs/).

Assuming you have Docker installed and configured to allow your user to run containers, do:

````
docker run -ti adamnovak/sequence-graphs:v0.01
```

This will put you in your own clone of this Git repository, on a dedicated Ubuntu 14.10 Linux container pre-configured with all the dependencies. The binaries will already have been built for you, and will reside in `createIndex/`. (Don't forget the `-ti` options, or you won't actually get a shell in the container.)

This gives you a Docker container of a particular stable version. If you want the latest and buggiest version, omit the `:v0.01`.

If you want to use the tools on particular files, you will need to download them inside the container, or look into configuring Docker to allow your container to access your computer's filesystem.

When you are done, leave the container's shell with:

```
exit
```

Later, to go back to your container, get its name (which will be of the form `<adjective>_<scientist>`) with:

```
docker ps -a
```

And then re-start it with

```
docker start -ai <container_name>
```

If you `docker run` again, you will create a fresh container, which won't have access to any of the files in the other container.

##Without Docker

Of course, the many benefits of Docker containerization are unavailable to those who need it most: poor grad students who are trying to use their university's decade-old CentOS servers with outdated versions of everything and no root access to get work done. 

If you don't want to use Docker, but you can actually use your distribution's package manager and install software from source as root on your machine, have a look at the `Dockerfile` and just do what Docker would do.

If you can't use Docker because you can't use your system's package manager, first complain to your system administrator, and then try following the [instructions for manual installation](https://github.com/adamnovak/sequence-graphs/blob/master/Manual%20Installation.md). 

#Running tests

Self-test code can be run with:

```
make check
```

Even if you can't run any of the other tools (which are themselves actually just glorified test programs), you can assure yourself that this repository contains code that actually does something.

#Running command-line tools

The package contains several command-line tools, all of which live in the `createIndex/` subdirectory:

* `createIndex/createIndex`: align and merge FASTA files into a sequence graph using context-driven mapping.
* `createIndex/mapReads`: index a single FASTA, and map reads from other FASTAs to it using context-driven mapping.
* `createIndex/evaluateMapability`: index a single FASTA, and determine the context lengths required to map to its positions under context-driven mapping.
* `createIndex/cactusMerge`: merge two pairs of `.c2h` and `.fa` files.

These tools are currently useful more for the debugging information and alignment statistics that they produce than for the actual alignments or indexes themselves.

##createIndex

The main command line tool is `createIndex`, somewhat confusingly hidden in the `createIndex` directory. Its job is to take one or more FASTA files and align them together using context-driven mapping, producing as output a compressed index to which new sequences can be aligned later. It has a large number of debug options, which are the real point of the tool: it is much more interesting to know how what portion of each sequence is getting aligned, and what the internal graph structure of the alignment is, than it is to align sequences to the completed index. Indeed, the tool to align sequences to the index is currently unfinished.

To run createIndex, do something like:

```
createIndex/createIndex testIndex --scheme greedy --mapType natural data/edit1.fa data/edit2.fa --alignment test.c2h --alignmentFasta test.fa
```

This will create an index in the directory testIndex (which, as said above, isn't useful for much at the moment) and will more importantly dump a Cactus2HAL-format alignment and associated FASTA file describing the structure of the merged sequence graph produced by mapping `data/edit2.fa` to `data/edit1.fa` and merging corresponding positions. The `.c2h` file can be read according to the specification [here](https://github.com/benedictpaten/cactus/blob/development/hal/impl/hal.c#L13) and the example [here] (https://github.com/adamnovak/sequence-graphs/blob/31992e4f89e2d33604c23df766fdee63fa9123e0/createIndex/pinchGraphUtil.hpp#L53). You can also use the [`halAppendCactusSubtree` tool](https://github.com/glennhickey/cactus2hal/blob/master/src/halAppendCactusSubtree.cpp), which must be installed seperately, to produce a somewhat-more-standard [HAL format](https://github.com/glennhickey/hal) alignment.


