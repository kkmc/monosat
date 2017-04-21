#!/bin/bash
FILES=/home/kevin/Workspace/cpsc449/monosat/examples/acyclic_gnf/acyclic/*
#FILES=/home/kevin/Workspace/cpsc449/monosat/examples/acyclic_gnf/new_instances/*

for f in $FILES
do
    c1="$(/home/kevin/Workspace/cpsc449/monosat/monosat -cycles=dfs $f)"
    #c2="$(/home/kevin/Workspace/cpsc449/monosat/monosat -cycles=pk $f)"
    c3="$(/home/kevin/Workspace/cpsc449/monosat/monosat -cycles=fc $f)"
 
    #if [ "${c1}" == "${c3}" ]; then
   # 	echo "Matching~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~: $f"
   # 	echo "${c1}"
   # 	echo "${c3}"
   # else
   #     echo "NOT A MATCH $f"
   # fi

    echo "dfs = ${c1}"
    # echo "pk  = ${c2}"
    echo "fc  = ${c3}"
    echo "$f"
    echo "==="
done
