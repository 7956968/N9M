#!/bin/bash

find ../ -name "*" | while read file;do touch $file;done

