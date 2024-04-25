#!/bin/bash

if [ $# -lt 1 ]; then
    echo "Usage: $0 <file>"
    exit 1
fi

file=$1
iterations=$2
numbers=$(wc -l < "$file")

# pocet procesoru
proc=$numbers
#preklad zdrojoveho souboru
mpic++ --prefix /usr/local/share/OpenMPI -o life life.cpp

#spusteni programu
mpirun --prefix /usr/local/share/OpenMPI  -np $proc life $file $iterations
