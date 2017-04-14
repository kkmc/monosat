#!/bin/bash
#FILES=/home/kevin/Workspace/cpsc449/monosat/examples/acyclic_gnf/acyclic/*
FILES=/home/kevin/Workspace/cpsc449/monosat/examples/acyclic_gnf/new_instances/*

for f in $FILES
do
    c1="$(/home/kevin/Workspace/cpsc449/monosat/monosat -cycles=dfs $f)"
    #c2="$(/home/kevin/Workspace/cpsc449/monosat/monosat -cycles=pk $f)"
    c3="$(/home/kevin/Workspace/cpsc449/monosat/monosat -cycles=fc $f)"
    
    echo "dfs = ${c1}"
    #echo "pk  = ${c2}"
    echo "fc  = ${c3}"
    echo "$f"
    echo "==="
done
